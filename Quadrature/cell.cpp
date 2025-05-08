#include "cell.h"

Cell::Cell(int id) : id(id), gain(0), locked(false), partition(0), movable(true) {}

void Cell::updateGain(int delta) {
    gain += delta;
}