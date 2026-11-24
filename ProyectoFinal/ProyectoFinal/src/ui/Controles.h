#pragma once

// ============================================================
//  Controles.h
//  Campos que NO cambian de valor con la rueda del mouse (tengan o no
//  el foco). Así, al hacer scroll por la página, no se
//  modifican pesos ni selecciones sin querer.
// ============================================================

#include <QComboBox>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QWheelEvent>

// Solo enteros: los pesos no admiten decimales. Solo se puede
// tipear un "-" al principio y dígitos (ni punto, ni coma, ni "+").
class SpinSinRueda : public QSpinBox {
public:
    static const int LIMITE = 99999; // 5 cifras: el rango y lo que se puede tipear coinciden
    explicit SpinSinRueda(QWidget* parent = nullptr) : QSpinBox(parent) {
        setRange(-LIMITE, LIMITE);
        setToolTip(QStringLiteral("Entero entre -99999 y 99999"));
        setFocusPolicy(Qt::StrongFocus);
        setButtonSymbols(QAbstractSpinBox::NoButtons);
        setInputMethodHints(Qt::ImhFormattedNumbersOnly); // teclado numérico en pantallas táctiles
        setKeyboardTracking(false); // el valor se aplica al confirmar, no en cada tecla (evita redibujar todo por dígito)
    }

protected:
    QValidator::State validate(QString& texto, int& posicion) const override {
        static const QRegularExpression permitido(QStringLiteral("^-?[0-9]{0,5}$"));
        if (!permitido.match(texto).hasMatch()) return QValidator::Invalid;
        return QSpinBox::validate(texto, posicion);
    }
    int valueFromText(const QString& texto) const override { return texto.toInt(); }
    QString textFromValue(int valor) const override { return QString::number(valor); } // sin separador de miles

    // La rueda nunca cambia el valor, ni con el foco puesto: siempre desplaza la página
    void wheelEvent(QWheelEvent* evento) override { evento->ignore(); }
};

class ComboSinRueda : public QComboBox {
public:
    explicit ComboSinRueda(QWidget* parent = nullptr) : QComboBox(parent) {
        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    void wheelEvent(QWheelEvent* evento) override { evento->ignore(); }
};

// Editor de peso para las celdas de la tabla de aristas: el SpinSinRueda solo existe mientras se edita
// (una tabla con un campo por celda hacía lentos todos los cambios cuando había muchas aristas).
class DelegadoPeso : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget* createEditor(QWidget* padre, const QStyleOptionViewItem&, const QModelIndex&) const override {
        auto* s = new SpinSinRueda(padre);
        s->setObjectName(QStringLiteral("enTabla"));
        return s;
    }
    void setEditorData(QWidget* editor, const QModelIndex& indice) const override {
        static_cast<QSpinBox*>(editor)->setValue(indice.data(Qt::EditRole).toInt());
    }
    void setModelData(QWidget* editor, QAbstractItemModel* modelo, const QModelIndex& indice) const override {
        auto* s = static_cast<QSpinBox*>(editor);
        s->interpretText();
        modelo->setData(indice, s->value(), Qt::EditRole);
    }
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& opcion, const QModelIndex&) const override {
        editor->setGeometry(opcion.rect.adjusted(6, 4, -6, -4));
    }
};
