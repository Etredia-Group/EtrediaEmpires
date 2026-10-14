# -- coding: utf-8 --
"""
Region Mapper — работа с двумя JSON: регионы (цвета) и страны (группы).
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
    QFormLayout
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
    img = QImage(path)
    if not img.isNull():
        return img, None
    with Image.open(path) as f:
        pil = f.convert("RGB")
    w, h = pil.size
    raw = pil.tobytes()
    del pil
    img = QImage(raw, w, h, w * 3, QImage.Format_RGB888)
    if img.isNull():
        raise RuntimeError("Не удалось декодировать изображение")
    return img, raw

def make_overview(img, max_side=4096):
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
    pixelPicked = pyqtSignal(int, int, tuple)
    zoomChanged = pyqtSignal(float)
    cursorMoved = pyqtSignal(int, int)

    LOD_SWITCH = 0.25
    MAX_SCALE = 64.0

    def __init__(self, image, raw_buffer, parent=None):
        super().__init__(parent)
        self.image = image
        self._raw = raw_buffer
        self.W = image.width()
        self.H = image.height()
        print("Размер карты: {} x {} пикселей".format(self.W, self.H))
        print("Создаю уменьшенную копию...")
        self.overview = make_overview(image)
        
        self.scale = 1.0
        self.offset = QPointF(0, 0)
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

    def _to_image(self, pos):
        x = math.floor((pos.x() - self.offset.x()) / self.scale)
        y = math.floor((pos.y() - self.offset.y()) / self.scale)
        return x, y

    def fitToWindow(self):
        vw, vh = self.width(), self.height()
        if vw < 2 or vh < 2: return
        s = min(vw / self.W, vh / self.H)
        self._fit_scale = s
        self.scale = s
        self.offset = QPointF((vw - self.W * s) / 2.0, (vh - self.H * s) / 2.0)
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
        ix = (point.x() - self.offset.x()) / self.scale
        iy = (point.y() - self.offset.y()) / self.scale
        self.scale = new_scale
        self.offset = QPointF(point.x() - ix * new_scale, point.y() - iy * new_scale)
        self.update()
        self.zoomChanged.emit(new_scale)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if not self._fitted: self.fitToWindow()

    def paintEvent(self, event):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(45, 45, 48))
        p.setRenderHint(QPainter.SmoothPixmapTransform, self.scale < 1.0)
        s = self.scale
        ox, oy = self.offset.x(), self.offset.y()
        
        if s < self.LOD_SWITCH:
            target = QRectF(ox, oy, self.W * s, self.H * s)
            p.drawPixmap(target, self.overview, QRectF(0, 0, self.overview.width(), self.overview.height()))
        else:
            x0 = max(0, int(math.floor(-ox / s)))
            y0 = max(0, int(math.floor(-oy / s)))
            x1 = min(self.W, int(math.ceil((self.width() - ox) / s)))
            y1 = min(self.H, int(math.ceil((self.height() - oy) / s)))
            if x1 > x0 and y1 > y0:
                src = QRect(x0, y0, x1 - x0, y1 - y0)
                target = QRectF(ox + x0 * s, oy + y0 * s, (x1 - x0) * s, (y1 - y0) * s)
                p.drawImage(target, self.image, QRectF(src))
                
        p.setPen(QPen(QColor(95, 95, 95), 1))
        p.drawRect(QRectF(ox, oy, self.W * s, self.H * s))
        
        if self.marker is not None and s >= 4:
            mx, my = self.marker
            p.setPen(QPen(QColor(255, 255, 255), 1))
            p.drawRect(QRectF(ox + mx * s, oy + my * s, s, s))
        p.end()

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
            self.offset = QPointF(self.offset.x() + d.x(), self.offset.y() + d.y())
            self.update()
        elif self._press_pos is not None:
            d = event.pos() - self._press_pos
            if d.manhattanLength() > 4: self._dragging = True
            if self._dragging:
                self.offset = QPointF(self._press_offset.x() + d.x(), self._press_offset.y() + d.y())
                self.update()
        else:
            x, y = self._to_image(event.pos())
            if 0 <= x < self.W and 0 <= y < self.H: self.cursorMoved.emit(x, y)
            else: self.cursorMoved.emit(-1, -1)

    def mouseReleaseEvent(self, event):
        if event.button() in (Qt.RightButton, Qt.MiddleButton):
            self._panning = False
            self.setCursor(Qt.ArrowCursor)
        elif event.button() == Qt.LeftButton and self._press_pos is not None:
            if not self._dragging: self._pick(event.pos())
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
        if event.key() in (Qt.Key_0, Qt.Key_F): self.fitToWindow()
        elif event.key() == Qt.Key_1: self.actualSize()
        elif event.key() in (Qt.Key_Plus, Qt.Key_Equal): self.zoomIn()
        elif event.key() == Qt.Key_Minus: self.zoomOut()
        else: super().keyPressEvent(event)

# ----------------------------------------------------------------------
# Окно
# ----------------------------------------------------------------------
class MainWindow(QMainWindow):
    def __init__(self, path):
        super().__init__()
        self.setWindowTitle("Region Mapper — " + os.path.basename(path))
        self.resize(1400, 850)

        # --- Файл 1: Регионы (плоский словарь) ---
        self.mapping = {}       # name -> "#RRGGBB"
        self.by_color = {}      # (r, g, b) -> name
        
        # --- Файл 2: Страны (вложенная структура) ---
        self.subjects_list = [] # список тегов для сохранения порядка
        self.subjects = {}      # tag -> {"color": str, "regions": list}

        self.current_rgb = None
        self._shown_once = False
        
        image, raw = load_map(path)
        self.viewer = MapViewer(image, raw)
        self.viewer.pixelPicked.connect(self.on_pixel)
        self.viewer.zoomChanged.connect(lambda s: self.zoom_label.setText("Масштаб: {:.4f}×".format(s)))
        self.viewer.cursorMoved.connect(lambda x, y: self.coord_label.setText("X: {}   Y: {}".format(x, y)))
        self._build_ui()

    def showEvent(self, event):
        super().showEvent(event)
        if not self._shown_once:
            self._shown_once = True
            QTimer.singleShot(0, self.viewer.fitToWindow)

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        root = QHBoxLayout(central)
        root.setContentsMargins(6, 6, 6, 6)
        splitter = QSplitter(Qt.Horizontal)

        # --- слева: карта ---
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

        # --- справа: панель управления ---
        panel = QWidget()
        pv = QVBoxLayout(panel)
        pv.setContentsMargins(8, 8, 8, 8)
        pv.setSpacing(6)
        
        # Блок текущего пикселя
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
        
        # ================= БЛОК РЕГИОНОВ (ФАЙЛ 1) =================
        pv.addSpacing(10)
        pv.addWidget(QLabel("1. Регионы (Название и цвет):"))
        self.name_input = QLineEdit()
        self.name_input.setPlaceholderText("Например: pepene")
        self.name_input.returnPressed.connect(self.commit)
        pv.addWidget(self.name_input)
        
        btn = QPushButton("Назначить / обновить цвет  [Enter]")
        btn.clicked.connect(self.commit)
        pv.addWidget(btn)
        
        self.list_widget = QListWidget()
        self.list_widget.itemDoubleClicked.connect(lambda it: self.rename_region(it.data(Qt.UserRole)))
        pv.addWidget(self.list_widget, 1)
        
        row2 = QHBoxLayout()
        b1 = QPushButton("Переименовать")
        b1.clicked.connect(lambda: self.rename_region())
        b2 = QPushButton("Удалить")
        b2.clicked.connect(self.delete_region)
        row2.addWidget(b1)
        row2.addWidget(b2)
        pv.addLayout(row2)

        # ================= БЛОК СТРАН (ФАЙЛ 2) =================
        pv.addSpacing(12)
        pv.addWidget(QLabel("2. Страны (Теги и группы регионов):"))
        
        subj_form = QFormLayout()
        subj_form.setContentsMargins(0, 0, 0, 0)
        subj_form.setSpacing(4)
        
        self.subject_tag_input = QLineEdit()
        self.subject_tag_input.setPlaceholderText("Тег (например, UCB)")
        subj_form.addRow("Тег:", self.subject_tag_input)
        
        self.subject_color_input = QLineEdit()
        self.subject_color_input.setPlaceholderText("Цвет (например, #E5EA5D)")
        subj_form.addRow("Цвет:", self.subject_color_input)
        pv.addLayout(subj_form)
        
        btn_use_color = QPushButton("Взять цвет текущего пикселя")
        btn_use_color.clicked.connect(self.use_current_color_for_subject)
        pv.addWidget(btn_use_color)
        
        btn_save_subject = QPushButton("Создать / Обновить страну")
        btn_save_subject.clicked.connect(self.commit_subject)
        pv.addWidget(btn_save_subject)
        
        self.subject_list = QListWidget()
        self.subject_list.currentItemChanged.connect(self.on_subject_selected)
        pv.addWidget(self.subject_list)
        
        pv.addWidget(QLabel("Регионы в выбранной стране:"))
        self.subject_regions_list = QListWidget()
        pv.addWidget(self.subject_regions_list)
        
        row_subj = QHBoxLayout()
        b_rem_reg = QPushButton("Удалить из страны")
        b_rem_reg.clicked.connect(self.remove_region_from_subject)
        b_del_subj = QPushButton("Удалить страну")
        b_del_subj.clicked.connect(self.delete_subject)
        row_subj.addWidget(b_rem_reg)
        row_subj.addWidget(b_del_subj)
        pv.addLayout(row_subj)

        # ================= КНОПКИ ФАЙЛОВ =================
        pv.addSpacing(12)
        pv.addWidget(QLabel("--- Файл регионов (JSON 1) ---"))
        row_files1 = QHBoxLayout()
        b_open_reg = QPushButton("Открыть регионы")
        b_open_reg.clicked.connect(self.load_regions_json)
        b_save_reg = QPushButton("Сохранить регионы")
        b_save_reg.setStyleSheet("background:#1565c0; color:white; font-weight:bold; padding:4px;")
        b_save_reg.clicked.connect(self.save_regions_json)
        row_files1.addWidget(b_open_reg)
        row_files1.addWidget(b_save_reg)
        pv.addLayout(row_files1)

        pv.addSpacing(6)
        pv.addWidget(QLabel("--- Файл стран (JSON 2) ---"))
        row_files2 = QHBoxLayout()
        b_open_subj = QPushButton("Открыть страны")
        b_open_subj.clicked.connect(self.load_subjects_json)
        b_save_subj = QPushButton("Сохранить страны")
        b_save_subj.setStyleSheet("background:#2e7d32; color:white; font-weight:bold; padding:4px;")
        b_save_subj.clicked.connect(self.save_subjects_json)
        row_files2.addWidget(b_open_subj)
        row_files2.addWidget(b_save_subj)
        pv.addLayout(row_files2)

        splitter.addWidget(panel)
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 0)
        splitter.setSizes([1000, 500])
        root.addWidget(splitter)

        self.coord_label = QLabel("X: —   Y: —")
        self.statusBar().addPermanentWidget(self.coord_label)
        self.statusBar().showMessage("ЛКМ — пиксель; ПКМ/ЛКМ-драг — панорама; колесо — зум")

    # ---------- Логика пикселя и регионов ----------
    def on_pixel(self, x, y, rgb):
        self.current_rgb = rgb
        hexv = rgb_to_hex(rgb)
        self.swatch.setStyleSheet("background:{}; border:1px solid #555;".format(hexv))
        text = "{}\nRGB ({}, {}, {})\nПиксель ({}, {})".format(hexv, rgb[0], rgb[1], rgb[2], x, y)
        owner = self.by_color.get(rgb)
        if owner:
            text += "\nУже назначен: " + owner
            self.name_input.setText(owner)
            self._select_in_list(owner)
        else:
            self.name_input.clear()
        self.color_info.setText(text)
        self.name_input.setFocus()
        self.name_input.selectAll()
        
        # Автодобавление в выбранную страну при клике
        current_subject_item = self.subject_list.currentItem()
        if current_subject_item:
            tag = current_subject_item.data(Qt.UserRole)
            region_name = self.by_color.get(rgb)
            if region_name:
                self.add_region_to_subject(tag, region_name)

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
        
        owner = self.by_color.get(rgb)
        if owner is not None and owner != name:
            ret = QMessageBox.question(self, "Цвет занят",
                "Цвет {} уже назначен региону «{}».\nПереназначить его региону «{}»?".format(hexv, owner, name),
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if ret != QMessageBox.Yes: return
            self.mapping.pop(owner, None)
            self.by_color.pop(rgb, None)
            
        if name in self.mapping:
            old_rgb = hex_to_rgb(self.mapping[name])
            if self.by_color.get(old_rgb) == name:
                self.by_color.pop(old_rgb, None)
                
        self.mapping[name] = hexv
        self.by_color[rgb] = name
        self._rebuild_list(select=name)
        self.name_input.clear()
        self.name_input.setFocus()
        
        current_subject_item = self.subject_list.currentItem()
        if current_subject_item:
            tag = current_subject_item.data(Qt.UserRole)
            self.add_region_to_subject(tag, name)

    def _rebuild_list(self, select=None):
        self.list_widget.clear()
        for i, (name, hexv) in enumerate(self.mapping.items(), 1):
            it = QListWidgetItem("{:>3}.  {}    {}".format(i, name, hexv))
            it.setData(Qt.UserRole, name)
            it.setBackground(QColor(hexv))
            c = QColor(hexv)
            bright = 0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue()
            it.setForeground(QColor(20, 20, 20) if bright > 140 else QColor(245, 245, 245))
            self.list_widget.addItem(it)
        if select is not None: self._select_in_list(select)

    def _select_in_list(self, name):
        for i in range(self.list_widget.count()):
            if self.list_widget.item(i).data(Qt.UserRole) == name:
                self.list_widget.setCurrentRow(i)
                return

    def rename_region(self, name=None):
        if name is None:
            it = self.list_widget.currentItem()
            if it is None: return
            name = it.data(Qt.UserRole)
        new, ok = QInputDialog.getText(self, "Переименовать регион", "Новое название:", QLineEdit.Normal, name)
        if not ok: return
        new = new.strip()
        if not new or new == name: return
        if new in self.mapping:
            QMessageBox.warning(self, "Дубликат", "Регион с таким названием уже есть.")
            return
            
        # Обновляем имя во всех странах
        for tag in self.subjects_list:
            regions = self.subjects[tag]["regions"]
            if name in regions:
                idx = regions.index(name)
                regions[idx] = new

        reordered = {}
        for k, v in self.mapping.items():
            reordered[new if k == name else k] = v
        self.mapping = reordered
        
        rgb = hex_to_rgb(self.mapping[new])
        self.by_color.pop(rgb, None)
        self.by_color[rgb] = new
        self._rebuild_list(select=new)
        if self.subject_list.currentItem(): self._update_subject_regions()

    def delete_region(self):
        it = self.list_widget.currentItem()
        if it is None: return
        name = it.data(Qt.UserRole)
        hexv = self.mapping.pop(name, None)
        if hexv is not None:
            rgb = hex_to_rgb(hexv)
            if self.by_color.get(rgb) == name: self.by_color.pop(rgb, None)
            
        # Удаляем из всех стран
        for tag in self.subjects_list:
            regions = self.subjects[tag]["regions"]
            if name in regions: regions.remove(name)
                
        self._rebuild_list()
        if self.subject_list.currentItem(): self._update_subject_regions()

    # ---------- Логика Стран ----------
    def use_current_color_for_subject(self):
        if self.current_rgb:
            self.subject_color_input.setText(rgb_to_hex(self.current_rgb))
        else:
            QMessageBox.information(self, "Цвет", "Сначала кликните по пикселю на карте.")

    def commit_subject(self):
        tag = self.subject_tag_input.text().strip()
        color = self.subject_color_input.text().strip().upper()
        if not tag:
            QMessageBox.warning(self, "Тег", "Введите тег страны.")
            return
        if not color.startswith("#") or len(color) != 7:
            QMessageBox.warning(self, "Цвет", "Введите корректный HEX цвет (например, #E5EA5D).")
            return
        
        if tag in self.subjects:
            self.subjects[tag]["color"] = color
        else:
            self.subjects_list.append(tag)
            self.subjects[tag] = {"color": color, "regions": []}
        
        self._rebuild_subject_list(select=tag)
        self.subject_tag_input.clear()
        self.subject_color_input.clear()

    def _rebuild_subject_list(self, select=None):
        self.subject_list.clear()
        for tag in self.subjects_list:
            data = self.subjects[tag]
            color = data["color"]
            text = "{}  ({})  [{} рег.]".format(tag, color, len(data["regions"]))
            it = QListWidgetItem(text)
            it.setData(Qt.UserRole, tag)
            it.setBackground(QColor(color))
            c = QColor(color)
            bright = 0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue()
            it.setForeground(QColor(20, 20, 20) if bright > 140 else QColor(245, 245, 245))
            self.subject_list.addItem(it)
        
        if select:
            for i in range(self.subject_list.count()):
                if self.subject_list.item(i).data(Qt.UserRole) == select:
                    self.subject_list.setCurrentRow(i)
                    break
        self._update_subject_regions()

    def on_subject_selected(self, current, previous):
        self._update_subject_regions()

    def _update_subject_regions(self):
        self.subject_regions_list.clear()
        item = self.subject_list.currentItem()
        if not item: return
        tag = item.data(Qt.UserRole)
        regions = self.subjects[tag]["regions"]
        for r in regions:
            self.subject_regions_list.addItem(r)

    def add_region_to_subject(self, tag, region_name):
        if tag not in self.subjects: return
        if region_name not in self.subjects[tag]["regions"]:
            self.subjects[tag]["regions"].append(region_name)
            if self.subject_list.currentItem() and self.subject_list.currentItem().data(Qt.UserRole) == tag:
                self._update_subject_regions()
                self._rebuild_subject_list(select=tag)

    def remove_region_from_subject(self):
        item = self.subject_list.currentItem()
        reg_item = self.subject_regions_list.currentItem()
        if not item or not reg_item: return
        tag = item.data(Qt.UserRole)
        region_name = reg_item.text()
        if region_name in self.subjects[tag]["regions"]:
            self.subjects[tag]["regions"].remove(region_name)
            self._update_subject_regions()
            self._rebuild_subject_list(select=tag)

    def delete_subject(self):
        item = self.subject_list.currentItem()
        if not item: return
        tag = item.data(Qt.UserRole)
        if tag in self.subjects_list: self.subjects_list.remove(tag)
        if tag in self.subjects: del self.subjects[tag]
        self._rebuild_subject_list()

    # ---------- Работа с ФАЙЛОМ 1 (Регионы) ----------
    def save_regions_json(self):
        if not self.mapping:
            QMessageBox.information(self, "Пусто", "Сначала добавьте регионы.")
            return
        path, _ = QFileDialog.getSaveFileName(self, "Сохранить регионы", "regions.json", "JSON (*.json)")
        if not path: return
        with open(path, "w", encoding="utf-8") as f:
            json.dump(self.mapping, f, ensure_ascii=False, indent=2)
            f.write("\n")
        self.statusBar().showMessage("Сохранено регионов: {} → {}".format(len(self.mapping), path), 8000)

    def load_regions_json(self):
        path, _ = QFileDialog.getOpenFileName(self, "Открыть регионы", "", "JSON (*.json)")
        if not path: return
        try:
            with open(path, encoding="utf-8") as f:
                data = json.load(f)
            if not isinstance(data, dict): raise ValueError("Ожидался JSON-объект (словарь)")
            
            mapping, by_color = {}, {}
            for name, value in data.items():
                if not isinstance(value, str): continue
                v = value.strip().upper()
                if len(v) != 7 or not v.startswith("#"): continue
                try: rgb = hex_to_rgb(v)
                except ValueError: continue
                name = str(name)
                if rgb in by_color: continue
                mapping[name] = rgb_to_hex(rgb)
                by_color[rgb] = name
        except Exception as e:
            QMessageBox.critical(self, "Ошибка чтения", str(e))
            return
            
        self.mapping = mapping
        self.by_color = by_color
        self._rebuild_list()
        self.statusBar().showMessage("Загружено регионов: {}".format(len(mapping)), 5000)

    # ---------- Работа с ФАЙЛОМ 2 (Страны) ----------
    def save_subjects_json(self):
        if not self.subjects_list:
            QMessageBox.information(self, "Пусто", "Сначала добавьте страны.")
            return
        path, _ = QFileDialog.getSaveFileName(self, "Сохранить страны", "subjects.json", "JSON (*.json)")
        if not path: return
        
        data = {"content": {"subjects": []}}
        for tag in self.subjects_list:
            subj = self.subjects[tag]
            data["content"]["subjects"].append({
                "tag": tag,
                "color": subj["color"],
                "regions": subj["regions"]
            })
            
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
            f.write("\n")
        self.statusBar().showMessage("Сохранено стран: {} → {}".format(len(self.subjects_list), path), 8000)

    def load_subjects_json(self):
        path, _ = QFileDialog.getOpenFileName(self, "Открыть страны", "", "JSON (*.json)")
        if not path: return
        try:
            with open(path, encoding="utf-8") as f:
                data = json.load(f)
                
            if "content" not in data or "subjects" not in data["content"]:
                raise ValueError("Ожидался формат с 'content' -> 'subjects'")
                
            self.subjects_list = []
            self.subjects = {}
            for subj in data["content"]["subjects"]:
                tag = str(subj.get("tag", ""))
                color = str(subj.get("color", "")).strip().upper()
                regions = [str(r) for r in subj.get("regions", [])]
                if tag and color.startswith("#") and len(color) == 7:
                    self.subjects_list.append(tag)
                    self.subjects[tag] = {"color": color, "regions": regions}
        except Exception as e:
            QMessageBox.critical(self, "Ошибка чтения", str(e))
            return
            
        self._rebuild_subject_list()
        self.statusBar().showMessage("Загружено стран: {}".format(len(self.subjects_list)), 5000)

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
        if not path: return
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