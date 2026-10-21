#pragma once

// ============================================================
//  Controles.h
//  Campos que NO cambian de valor con la rueda del mouse si no
//  tienen el foco. Así, al hacer scroll por la página, no se
//  modifican pesos ni selecciones sin querer.
// ============================================================

#include <QComboBox>
#include <QSpinBox>
#include <QWheelEvent>

// Solo enteros: los pesos no admiten decimales.
class SpinSinRueda : public QSpinBox {
public:
    explicit SpinSinRueda(QWidget* parent = nullptr) : QSpinBox(parent) {
        setRange(-100000, 100000);
        setFocusPolicy(Qt::StrongFocus);
        setButtonSymbols(QAbstractSpinBox::NoButtons);
    }

protected:
    void wheelEvent(QWheelEvent* evento) override {
        if (!hasFocus()) {
            evento->ignore(); // la rueda pasa a la página
            return;
        }
        QSpinBox::wheelEvent(evento);
    }
};

class ComboSinRueda : public QComboBox {
public:
    explicit ComboSinRueda(QWidget* parent = nullptr) : QComboBox(parent) {
        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    void wheelEvent(QWheelEvent* evento) override {
        if (!hasFocus()) {
            evento->ignore();
            return;
        }
        QComboBox::wheelEvent(evento);
    }
};
