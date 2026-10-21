#pragma once

// ============================================================
//  Tema.h
//  Colores y hoja de estilo, copiados de la paleta del
//  prototipo HTML "Caminos en grafos".
// ============================================================

#include <QColor>
#include <QString>

namespace tema {

inline QColor fondo()      { return QColor("#eef1f4"); } // --bg
inline QColor panel()      { return QColor("#ffffff"); } // --panel
inline QColor tinta()      { return QColor("#1d2733"); } // --ink
inline QColor apagado()    { return QColor("#6b7885"); } // --mute
inline QColor linea()      { return QColor("#d3dae1"); } // --line
inline QColor camino()     { return QColor("#0b7a75"); } // --path
inline QColor caminoFondo(){ return QColor("#d9f0ee"); } // --pathbg
inline QColor obligatorio(){ return QColor("#b86e00"); } // --must
inline QColor bloqueado()  { return QColor("#b3261e"); } // --bad

inline QString hojaDeEstilo() {
    return QString::fromUtf8(R"(
QWidget { color: #1d2733; font-family: "Segoe UI"; font-size: 10pt; }
QMainWindow, QWidget#fondo, QScrollArea#scrollIzquierdo, QWidget#contenidoIzquierdo { background: #eef1f4; }
QScrollArea { border: none; background: transparent; }
QFrame#tarjeta { background: #ffffff; border: 1px solid #d3dae1; border-radius: 10px; }
QFrame#tarjeta QLabel { background: transparent; }
QLabel#titulo { font-size: 17pt; font-weight: 700; }
QLabel#subtitulo { color: #6b7885; }
QLabel#tituloTarjeta { font-size: 11pt; font-weight: 600; }
QLabel#pista { color: #6b7885; font-size: 9pt; }
QLabel#costo { color: #0b7a75; font-size: 24pt; font-weight: 700; }
QLabel#error { color: #b3261e; font-weight: 600; }
QLineEdit, QDoubleSpinBox, QComboBox {
    background: #eef1f4; border: 1px solid #d3dae1; border-radius: 6px; padding: 4px 6px; min-height: 20px;
}
QLineEdit:focus, QDoubleSpinBox:focus, QComboBox:focus { border: 1px solid #0b7a75; }
QComboBox QAbstractItemView { background: #ffffff; selection-background-color: #d9f0ee; selection-color: #1d2733; }
QPushButton {
    background: #ffffff; border: 1px solid #d3dae1; border-radius: 6px; padding: 6px 12px;
}
QPushButton:hover { border-color: #0b7a75; }
QPushButton:pressed { background: #d9f0ee; }
QPushButton#principal { background: #1d2733; color: #ffffff; border-color: #1d2733; font-weight: 600; }
QPushButton#principal:hover { background: #0b7a75; border-color: #0b7a75; }
QToolButton#quitar { border: none; color: #6b7885; padding: 0 6px; background: transparent; }
QToolButton#quitar:hover { color: #b3261e; }
QTableWidget { background: #ffffff; border: none; gridline-color: #d3dae1; }
QTableWidget::item { padding: 2px 4px; border-bottom: 1px solid #d3dae1; }
QHeaderView::section { background: #ffffff; color: #6b7885; border: none; border-bottom: 1px solid #d3dae1; padding: 4px; font-size: 9pt; }
QTableCornerButton::section { background: #ffffff; border: none; }
QGraphicsView { background: #ffffff; border: 1px solid #d3dae1; border-radius: 10px; }
QSplitter::handle { background: #eef1f4; }
QCheckBox { spacing: 6px; background: transparent; }
QMenu { background: #ffffff; border: 1px solid #d3dae1; }
QMenu::item { padding: 6px 18px; }
QMenu::item:selected { background: #d9f0ee; color: #1d2733; }
)");
}

} // namespace tema
