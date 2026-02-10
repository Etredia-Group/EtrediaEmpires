#ifndef ETREDIAEMPIRES_UTILS_HPP
#define ETREDIAEMPIRES_UTILS_HPP

#define MAP_WORKER_COMMAND(TCommand, method) [this](TCommand &cmd) {this->method(cmd);}

#endif //ETREDIAEMPIRES_UTILS_HPP
