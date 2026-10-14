# -*- coding: utf-8 -*-
"""
Region Mapper — большая цветовая карта -> JSON {region: "#RRGGBB"}.
Зависимости: pip install PyQt5 Pillow
Запуск:     python nudge.py map.bmp
"""

import sys
import os
import json
import math

from PyQt5.QtCore import Qt, QPoint, QPointF, QRect, QRectF, QTimer, pyqtSignal
from PyQt5.QtGui import QImage, QPixmap, QPainter, QColor, QPen, QKeySequence
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
    QLabel, QLineEdit, QPushButton, QListWidget, QListWidgetItem,
    QSplitter, QFileDialog, QMessageBox, QInputDialog, QShortcut,
)
from PIL import Image

Image.MAX_IMAGE_PIXELS = None


# ----------------------------------------------------------------------
# Утилиты
# ----------------------------------------------------------------------

def rgb_to_hex(rgb):
    return "#{:02X}{:02X}{:02X}".format(*rgb)


def hex_to_rgb(s):
    s = s.lstrip("#")
    return int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16)


def load_map(path):
    """Возвращает (QImage, buffer). buffer нужно держать живым во втором случае."""

    # Штатный загрузчик Qt: нет лимита Pillow, нет лишних копий памяти.
    img = QImage(path)
    if not img.isNull():
        return img, None

    # Запасной путь через Pillow.
    with Image.open(path) as f:
        pil = f.convert("RGB")
    w, h = pil.size
    raw = pil.tobytes()
    del pil

    # ВАЖНО: этот конструктор QImage НЕ копирует данные.
    # Буфер raw обязан жить столько же, сколько живёт QImage.
    img = QImage(raw, w, h, w * 3, QImage.Format_RGB888)
    if img.isNull():
        raise RuntimeError("Не удалось декодировать изображение")
    return img, raw


def make_overview(img, max_side=4096):
    """Маленькая копия для режима «вся карта на экране»."""
    w, h = img.width(), img.height()
    k = min(1.0, max_side / float(max(w, h)))
    if k >= 1.0:
        small = img
    else:
        small = img.scaled(max(1, int(w * k)), max(1, int(h * k)),
                           Qt.IgnoreAspectRatio, Qt.SmoothTransformation)
    return QPixmap.fromImage(small)


# ----------------------------------------------------------------------
# Просмотр карты
# ----------------------------------------------------------------------

class MapViewer(QWidget):
    pixelPicked = pyqtSignal(int, int, tuple)   # x, y, (r, g, b)
    zoomChanged = pyqtSignal(float)
    cursorMoved = pyqtSignal(int, int)

    LOD_SWITCH = 0.25   # ниже этого масштаба рисуем уменьшенную копию
    MAX_SCALE = 64.0

    def __init__(self, image, raw_buffer, parent=None):
        super().__init__(parent)
        self.image = image
        self._raw = raw_buffer          # держим буфер Pillow живым
        self.W = image.width()
        self.H = image.height()

        print("Размер карты: {} x {} пикселей".format(self.W, self.H))
        print("Создаю уменьшенную копию...")
        self.overview = make_overview(image)

        self.scale = 1.0                # экранных пикселей на пиксель карты
        self.offset = QPointF(0, 0)     # куда смещён левый верхний угол карты
        self._fit_scale = 1.0
        self._fitted = False

        self._panning = False
        self._pan_last = QPoint()
        self._press_pos = None
        self._press_offset = QPointF()
        self._dragging = False
        self.marker = None

        self.setMouseTracking(True)
        self.setFocusPolicy(Qt.StrongFocus)
        self.setMinimumSize(200, 120)

    # ---------- координаты ----------

    def _to_image(self, pos):
        x = math.floor((pos.x() - self.offset.x()) / self.scale)
        y = math.floor((pos.y() - self.offset.y()) / self.scale)
        return x, y

    # ---------- масштаб ----------

    def fitToWindow(self):
        vw, vh = self.width(), self.height()
        if vw < 2 or vh < 2:
            return
        s = min(vw / self.W, vh / self.H)
        self._fit_scale = s
        self.scale = s
        self.offset = QPointF((vw - self.W * s) / 2.0,
                              (vh - self.H * s) / 2.0)
        self._fitted = True
        self.update()
        self.zoomChanged.emit(s)

    def actualSize(self):
        self._zoom_at(QPointF(self.width() / 2, self.height() / 2), 1.0)

    def zoomIn(self):
        self._zoom_at(QPointF(self.width() / 2, self.height() / 2), self.scale * 1.5)

    def zoomOut(self):
        self._zoom_at(QPointF(self.width() / 2, self.height() / 2), self.scale / 1.5)

    def _zoom_at(self, point, new_scale):
        new_scale = max(self._fit_scale * 0.25, min(self.MAX_SCALE, new_scale))
        # Точка карты под курсором должна остаться под курсором.
        ix = (point.x() - self.offset.x()) / self.scale
        iy = (point.y() - self.offset.y()) / self.scale
        self.scale = new_scale
        self.offset = QPointF(point.x() - ix * new_scale,
                              point.y() - iy * new_scale)
        self.update()
        self.zoomChanged.emit(new_scale)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if not self._fitted:
            self.fitToWindow()

    # ---------- отрисовка ----------

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(45, 45, 48))
        p.setRenderHint(QPainter.SmoothPixmapTransform, self.scale < 1.0)

        s = self.scale
        ox, oy = self.offset.x(), self.offset.y()

        if s < self.LOD_SWITCH:
            # Далеко: рисуем маленькую копию, Qt сам обрежет по виджету.
            target = QRectF(ox, oy, self.W * s, self.H * s)
            p.drawPixmap(target, self.overview,
                         QRectF(0, 0, self.overview.width(), self.overview.height()))
        else:
            # Близко: берём из полного разрешения ТОЛЬКО видимый прямоугольник.
            x0 = max(0, int(math.floor(-ox / s)))
            y0 = max(0, int(math.floor(-oy / s)))
            x1 = min(self.W, int(math.ceil((self.width() - ox) / s)))
            y1 = min(self.H, int(math.ceil((self.height() - oy) / s)))

            if x1 > x0 and y1 > y0:
                src = QRect(x0, y0, x1 - x0, y1 - y0)
                target = QRectF(ox + x0 * s, oy + y0 * s,
                                (x1 - x0) * s, (y1 - y0) * s)
                p.drawImage(target, self.image, QRectF(src))

        # Рамка карты
        p.setPen(QPen(QColor(95, 95, 95), 1))
        p.drawRect(QRectF(ox, oy, self.W * s, self.H * s))

        # Метка последнего выбранного пикселя
        if self.marker is not None and s >= 4:
            mx, my = self.marker
            p.setPen(QPen(QColor(255, 255, 255), 1))
            p.drawRect(QRectF(ox + mx * s, oy + my * s, s, s))
        p.end()

    # ---------- мышь ----------

    def wheelEvent(self, event):
        d = event.angleDelta().y()
        if d:
            factor = 1.25 if d > 0 else 1.0 / 1.25
            self._zoom_at(QPointF(event.pos()), self.scale * factor)

    def mousePressEvent(self, event):
        if event.button() in (Qt.RightButton, Qt.MiddleButton):
            self._panning = True
            self._pan_last = event.pos()
            self.setCursor(Qt.ClosedHandCursor)
        elif event.button() == Qt.LeftButton:
            self._press_pos = event.pos()
            self._press_offset = QPointF(self.offset)
            self._dragging = False

    def mouseMoveEvent(self, event):
        if self._panning:
            d = event.pos() - self._pan_last
            self._pan_last = event.pos()
            self.offset = QPointF(self.offset.x() + d.x(),
                                  self.offset.y() + d.y())
            self.update()
        elif self._press_pos is not None:
            d = event.pos() - self._press_pos
            if d.manhattanLength() > 4:
                self._dragging = True
            if self._dragging:
                self.offset = QPointF(self._press_offset.x() + d.x(),
                                      self._press_offset.y() + d.y())
                self.update()
        else:
            x, y = self._to_image(event.pos())
            if 0 <= x < self.W and 0 <= y < self.H:
                self.cursorMoved.emit(x, y)
            else:
                self.cursorMoved.emit(-1, -1)

    def mouseReleaseEvent(self, event):
        if event.button() in (Qt.RightButton, Qt.MiddleButton):
            self._panning = False
            self.setCursor(Qt.ArrowCursor)
        elif event.button() == Qt.LeftButton and self._press_pos is not None:
            if not self._dragging:
                self._pick(event.pos())
            self._press_pos = None
            self._dragging = False

    def _pick(self, pos):
        x, y = self._to_image(pos)
        if 0 <= x < self.W and 0 <= y < self.H:
            c = self.image.pixelColor(x, y)
            self.marker = (x, y)
            self.update()
            self.pixelPicked.emit(x, y, (c.red(), c.green(), c.blue()))

    def keyPressEvent(self, event):
        if event.key() in (Qt.Key_0, Qt.Key_F):
            self.fitToWindow()
        elif event.key() == Qt.Key_1:
            self.actualSize()
        elif event.key() in (Qt.Key_Plus, Qt.Key_Equal):
            self.zoomIn()
        elif event.key() == Qt.Key_Minus:
            self.zoomOut()
        else:
            super().keyPressEvent(event)


# ----------------------------------------------------------------------
# Окно
# ----------------------------------------------------------------------

class MainWindow(QMainWindow):
    def __init__(self, path):
        super().__init__()
        self.setWindowTitle("Region Mapper — " + os.path.basename(path))
        self.resize(1400, 850)

        # Обычный dict: в Python 3.7+ сохраняет порядок вставки.
        self.mapping = {}       # name -> "#RRGGBB"
        self.by_color = {}      # (r, g, b) -> name
        self.current_rgb = None
        self._shown_once = False

        image, raw = load_map(path)
        self.viewer = MapViewer(image, raw)
        self.viewer.pixelPicked.connect(self.on_pixel)
        self.viewer.zoomChanged.connect(
            lambda s: self.zoom_label.setText("Масштаб: {:.4f}×".format(s)))
        self.viewer.cursorMoved.connect(
            lambda x, y: self.coord_label.setText("X: {}   Y: {}".format(x, y)))

        self._build_ui()

    def showEvent(self, event):
        super().showEvent(event)
        if not self._shown_once:
            self._shown_once = True
            # Финальный фит уже после того, как layout раздал размеры.
            QTimer.singleShot(0, self.viewer.fitToWindow)

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        root = QHBoxLayout(central)
        root.setContentsMargins(6, 6, 6, 6)
        splitter = QSplitter(Qt.Horizontal)

        # --- слева: тулбар + карта ---
        left = QWidget()
        lv = QVBoxLayout(left)
        lv.setContentsMargins(0, 0, 0, 0)
        tb = QHBoxLayout()
        for text, slot in (("Вписать (0)", self.viewer.fitToWindow),
                           ("1:1", self.viewer.actualSize),
                           ("+", self.viewer.zoomIn),
                           ("−", self.viewer.zoomOut)):
            b = QPushButton(text)
            b.clicked.connect(slot)
            tb.addWidget(b)
        self.zoom_label = QLabel("Масштаб: —")
        tb.addWidget(self.zoom_label)
        tb.addStretch()
        lv.addLayout(tb)
        lv.addWidget(self.viewer, 1)
        splitter.addWidget(left)

        # --- справа: панель разметки ---
        panel = QWidget()
        pv = QVBoxLayout(panel)
        pv.setContentsMargins(8, 8, 8, 8)
        pv.setSpacing(6)

        pv.addWidget(QLabel("Текущий пиксель:"))
        row = QHBoxLayout()
        self.swatch = QLabel()
        self.swatch.setFixedSize(64, 44)
        self.swatch.setStyleSheet("background:#808080; border:1px solid #555;")
        row.addWidget(self.swatch)
        self.color_info = QLabel("Кликните ЛКМ по карте")
        self.color_info.setWordWrap(True)
        row.addWidget(self.color_info, 1)
        pv.addLayout(row)

        pv.addSpacing(10)
        pv.addWidget(QLabel("Название региона:"))
        self.name_input = QLineEdit()
        self.name_input.setPlaceholderText("Например: pepene")
        self.name_input.returnPressed.connect(self.commit)
        pv.addWidget(self.name_input)

        btn = QPushButton("Назначить / обновить цвет  [Enter]")
        btn.clicked.connect(self.commit)
        pv.addWidget(btn)

        pv.addSpacing(12)
        pv.addWidget(QLabel("Регионы (порядок = порядок в JSON):"))
        self.list_widget = QListWidget()
        self.list_widget.itemDoubleClicked.connect(
            lambda it: self.rename_region(it.data(Qt.UserRole)))
        pv.addWidget(self.list_widget, 1)

        row2 = QHBoxLayout()
        b1 = QPushButton("Переименовать")
        b1.clicked.connect(lambda: self.rename_region())
        b2 = QPushButton("Удалить")
        b2.clicked.connect(self.delete_region)
        row2.addWidget(b1)
        row2.addWidget(b2)
        pv.addLayout(row2)

        pv.addSpacing(12)
        b_open = QPushButton("Открыть существующий JSON")
        b_open.clicked.connect(self.load_json)
        pv.addWidget(b_open)

        b_save = QPushButton("Сохранить JSON  [Ctrl+S]")
        b_save.setStyleSheet("background:#2e7d32; color:white; "
                             "font-weight:bold; padding:6px;")
        b_save.clicked.connect(self.save_json)
        pv.addWidget(b_save)

        splitter.addWidget(panel)
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 0)
        splitter.setSizes([1000, 330])
        root.addWidget(splitter)

        self.coord_label = QLabel("X: —   Y: —")
        self.statusBar().addPermanentWidget(self.coord_label)
        self.statusBar().showMessage(
            "ЛКМ — выбрать пиксель; ЛКМ-драг / ПКМ — панорама; "
            "колесо — зум; 0 — вписать; 1 — 1:1")

        QShortcut(QKeySequence("Ctrl+S"), self, self.save_json)

    # ---------- выбор пикселя ----------

    def on_pixel(self, x, y, rgb):
        self.current_rgb = rgb
        hexv = rgb_to_hex(rgb)
        self.swatch.setStyleSheet("background:{}; border:1px solid #555;".format(hexv))

        text = "{}\nRGB ({}, {}, {})\nПиксель ({}, {})".format(
            hexv, rgb[0], rgb[1], rgb[2], x, y)

        owner = self.by_color.get(rgb)
        if owner:
            text += "\nУже назначен: " + owner
            self.name_input.setText(owner)   # сразу готово к правке
            self._select_in_list(owner)
        else:
            self.name_input.clear()

        self.color_info.setText(text)
        self.name_input.setFocus()
        self.name_input.selectAll()

    # ---------- запись / правка маппинга ----------

    def commit(self):
        name = self.name_input.text().strip()
        if not name:
            QMessageBox.warning(self, "Название", "Введите название региона.")
            return
        if self.current_rgb is None:
            QMessageBox.warning(self, "Цвет", "Сначала кликните по пикселю на карте.")
            return

        rgb = self.current_rgb
        hexv = rgb_to_hex(rgb)

        # Цвет уже занят другим регионом?
        owner = self.by_color.get(rgb)
        if owner is not None and owner != name:
            ret = QMessageBox.question(
                self, "Цвет занят",
                "Цвет {} уже назначен региону «{}».\n"
                "Переназначить его региону «{}»?".format(hexv, owner, name),
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if ret != QMessageBox.Yes:
                return
            self.mapping.pop(owner, None)
            self.by_color.pop(rgb, None)

        # Освобождаем старый цвет, если регион редактируется.
        if name in self.mapping:
            old_rgb = hex_to_rgb(self.mapping[name])
            if self.by_color.get(old_rgb) == name:
                self.by_color.pop(old_rgb, None)

        # Присваивание СУЩЕСТВУЮЩЕМУ ключу не меняет его позицию в dict.
        self.mapping[name] = hexv
        self.by_color[rgb] = name

        self._rebuild_list(select=name)
        self.name_input.clear()
        self.name_input.setFocus()

    def _rebuild_list(self, select=None):
        self.list_widget.clear()
        for i, (name, hexv) in enumerate(self.mapping.items(), 1):
            it = QListWidgetItem("{:>3}.  {}    {}".format(i, name, hexv))
            it.setData(Qt.UserRole, name)
            it.setBackground(QColor(hexv))
            c = QColor(hexv)
            bright = 0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue()
            it.setForeground(QColor(20, 20, 20) if bright > 140
                             else QColor(245, 245, 245))
            self.list_widget.addItem(it)
        if select is not None:
            self._select_in_list(select)

    def _select_in_list(self, name):
        for i in range(self.list_widget.count()):
            if self.list_widget.item(i).data(Qt.UserRole) == name:
                self.list_widget.setCurrentRow(i)
                return

    def rename_region(self, name=None):
        if name is None:
            it = self.list_widget.currentItem()
            if it is None:
                return
            name = it.data(Qt.UserRole)

        new, ok = QInputDialog.getText(self, "Переименовать регион",
                                       "Новое название:", QLineEdit.Normal, name)
        if not ok:
            return
        new = new.strip()
        if not new or new == name:
            return
        if new in self.mapping:
            QMessageBox.warning(self, "Дубликат", "Регион с таким названием уже есть.")
            return

        # Пересобираем dict, сохраняя позицию переименованного элемента.
        reordered = {}
        for k, v in self.mapping.items():
            reordered[new if k == name else k] = v
        self.mapping = reordered

        rgb = hex_to_rgb(self.mapping[new])
        self.by_color.pop(rgb, None)
        self.by_color[rgb] = new
        self._rebuild_list(select=new)

    def delete_region(self):
        it = self.list_widget.currentItem()
        if it is None:
            return
        name = it.data(Qt.UserRole)
        hexv = self.mapping.pop(name, None)
        if hexv is not None:
            rgb = hex_to_rgb(hexv)
            if self.by_color.get(rgb) == name:
                self.by_color.pop(rgb, None)
        self._rebuild_list()

    # ---------- JSON ----------

    def save_json(self):
        if not self.mapping:
            QMessageBox.information(self, "Пусто", "Сначала добавьте регион.")
            return
        path, _ = QFileDialog.getSaveFileName(self, "Сохранить маппинг",
                                              "regions.json", "JSON (*.json)")
        if not path:
            return
        with open(path, "w", encoding="utf-8") as f:
            json.dump(self.mapping, f, ensure_ascii=False, indent=2)
            f.write("\n")
        self.statusBar().showMessage(
            "Сохранено регионов: {} → {}".format(len(self.mapping), path), 8000)

    def load_json(self):
        path, _ = QFileDialog.getOpenFileName(self, "Открыть маппинг",
                                              "", "JSON (*.json)")
        if not path:
            return
        try:
            with open(path, encoding="utf-8") as f:
                data = json.load(f)
            if not isinstance(data, dict):
                raise ValueError("Ожидался JSON-объект")

            mapping, by_color = {}, {}
            for name, value in data.items():
                if not isinstance(value, str):
                    continue
                v = value.strip()
                if len(v) != 7 or not v.startswith("#"):
                    continue
                try:
                    rgb = hex_to_rgb(v)
                except ValueError:
                    continue
                name = str(name)
                if rgb in by_color:
                    continue
                mapping[name] = rgb_to_hex(rgb)
                by_color[rgb] = name
        except Exception as e:
            QMessageBox.critical(self, "Ошибка чтения", str(e))
            return

        self.mapping = mapping
        self.by_color = by_color
        self._rebuild_list()
        self.statusBar().showMessage("Загружено регионов: {}".format(len(mapping)), 5000)


# ----------------------------------------------------------------------

def main():
    app = QApplication(sys.argv)
    app.setApplicationName("Region Mapper")

    if len(sys.argv) > 1:
        path = sys.argv[1]
    else:
        path, _ = QFileDialog.getOpenFileName(
            None, "Открыть карту", "",
            "Изображения (*.bmp *.png *.jpg *.jpeg *.tif *.tiff);;Все файлы (*)")
        if not path:
            return

    if not os.path.isfile(path):
        QMessageBox.critical(None, "Файл не найден", path)
        return

    try:
        win = MainWindow(path)
    except Exception as exc:
        QMessageBox.critical(None, "Не удалось открыть карту", repr(exc))
        raise

    win.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()