#pragma once

// ============================================================
//  Controles.h
//  Campos que NO cambian de valor con la rueda del mouse si no
//  tienen el foco. Así, al hacer scroll por la página, no se
//  modifican pesos ni selecciones sin querer.
// ============================================================

#include <QComboBox>
#include <QRegularExpression>
#include <QSpinBox>
#include <QWheelEvent>

// Solo enteros: los pesos no admiten decimales. Solo se puede
// tipear un "-" al principio y dígitos (ni punto, ni coma, ni "+").
class SpinSinRueda : public QSpinBox {
public:
    explicit SpinSinRueda(QWidget* parent = nullptr) : QSpinBox(parent) {
        setRange(-100000, 100000);
        setFocusPolicy(Qt::StrongFocus);
        setButtonSymbols(QAbstractSpinBox::NoButtons);
    }

protected:
    QValidator::State validate(QString& texto, int& posicion) const override {
        static const QRegularExpression permitido(QStringLiteral("^-?[0-9]*$"));
        if (!permitido.match(texto).hasMatch()) return QValidator::Invalid;
        return QSpinBox::validate(texto, posicion);
    }
    int valueFromText(const QString& texto) const override { return texto.toInt(); }
    QString textFromValue(int valor) const override { return QString::number(valor); } // sin separador de miles

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
