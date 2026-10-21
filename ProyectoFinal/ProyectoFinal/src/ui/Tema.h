#pragma once

// ============================================================
//  Tema.h
//  Tema oscuro con detalles rosas. Todos los colores de la
//  aplicación salen de acá.
// ============================================================

#include <QColor>
#include <QString>

namespace tema {

// ---- superficies ----
inline QColor fondo()       { return QColor("#121016"); } // fondo de la ventana
inline QColor panel()       { return QColor("#1b1821"); } // tarjetas y lienzo
inline QColor campo()       { return QColor("#25212c"); } // inputs, nodos
inline QColor linea()       { return QColor("#332d3c"); } // bordes suaves
// ---- texto ----
inline QColor tinta()       { return QColor("#ede7f2"); }
inline QColor apagado()     { return QColor("#9b90a8"); }
inline QColor tenue()       { return QColor("#5f566b"); } // aristas comunes
// ---- acentos ----
inline QColor camino()      { return QColor("#ff5fa2"); } // rosa principal
inline QColor caminoFondo() { return QColor("#3b1a2d"); } // rosa oscuro
inline QColor obligatorio() { return QColor("#ffb86b"); } // ámbar
inline QColor bloqueado()   { return QColor("#ff6b6b"); } // rojo

// Hoja de estilo. "iconos" es la carpeta con check.png y flecha.png
// (se generan al iniciar, ver ProyectoFinal.cpp).
inline QString hojaDeEstilo(const QString& iconos) {
    QString css = QString::fromUtf8(R"(
* { font-family: "Segoe UI"; }
QWidget { color: #ede7f2; font-size: 10pt; }
QMainWindow, QWidget#fondo, QScrollArea#scrollPagina, QWidget#pagina { background: #121016; }
QScrollArea { border: none; background: transparent; }
QScrollArea > QWidget > QWidget { background: transparent; }

/* ---------- tarjetas ---------- */
QFrame#tarjeta { background: #1b1821; border: 1px solid #2a2532; border-radius: 14px; }
QFrame#tarjeta QLabel, QFrame#tarjeta QCheckBox { background: transparent; }

/* ---------- textos ---------- */
QLabel#titulo { font-size: 20pt; font-weight: 700; color: #ede7f2; }
QLabel#subtitulo { color: #9b90a8; font-size: 10pt; }
QLabel#etiquetaMateria { color: #ff5fa2; background: #3b1a2d; border-radius: 10px; padding: 3px 10px; font-size: 9pt; font-weight: 600; }
QLabel#tituloTarjeta { font-size: 12pt; font-weight: 600; color: #ede7f2; }
QLabel#rotulo { color: #9b90a8; font-size: 9pt; font-weight: 600; }
QLabel#pista { color: #9b90a8; font-size: 9pt; }
QLabel#costo { color: #ff5fa2; font-size: 30pt; font-weight: 700; }
QLabel#error { color: #ff6b6b; font-weight: 600; font-size: 10pt; }
QLabel#chip { color: #ff5fa2; background: #3b1a2d; border: 1px solid #5a2945; border-radius: 9px; padding: 4px 12px; font-weight: 600; }
QLabel#flechaChip { color: #5f566b; font-size: 11pt; }

/* ---------- campos ---------- */
QLineEdit, QSpinBox, QComboBox {
    background: #25212c; color: #ede7f2; border: 1px solid #332d3c; border-radius: 8px;
    padding: 7px 10px; min-height: 18px; selection-background-color: #ff5fa2; selection-color: #121016;
}
QLineEdit:hover, QSpinBox:hover, QComboBox:hover { border-color: #4a4155; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: #ff5fa2; }
QLineEdit:disabled, QSpinBox:disabled { color: #5f566b; background: #1f1b25; }
QComboBox { padding-right: 28px; }
QComboBox::drop-down { border: none; width: 26px; subcontrol-origin: padding; subcontrol-position: center right; }
QComboBox::down-arrow { image: url("@ICONOS@/flecha.png"); width: 10px; height: 6px; }
QComboBox QAbstractItemView {
    background: #25212c; border: 1px solid #332d3c; border-radius: 8px; padding: 4px;
    selection-background-color: #3b1a2d; selection-color: #ff5fa2; outline: none;
}
QSpinBox#enTabla, QLineEdit#enTabla { padding: 4px 8px; border-radius: 6px; background: #211d27; }

/* ---------- botones ---------- */
QPushButton {
    background: #25212c; color: #ede7f2; border: 1px solid #332d3c; border-radius: 8px; padding: 8px 16px; font-weight: 600;
}
QPushButton:hover { border-color: #ff5fa2; color: #ff8fbf; }
QPushButton:pressed { background: #3b1a2d; }
QPushButton#principal { background: #ff5fa2; color: #1a0f16; border: none; }
QPushButton#principal:hover { background: #ff7cb4; color: #1a0f16; }
QPushButton#principal:pressed { background: #e04a8a; }
QPushButton#peligro:hover { border-color: #ff6b6b; color: #ff6b6b; }
QToolButton#quitar { border: none; color: #6f6580; padding: 2px 8px; background: transparent; font-size: 11pt; }
QToolButton#quitar:hover { color: #ff6b6b; }

/* ---------- checkboxes ---------- */
QCheckBox { spacing: 8px; color: #ede7f2; }
QCheckBox::indicator { width: 18px; height: 18px; border-radius: 5px; border: 1px solid #4a4155; background: #25212c; }
QCheckBox::indicator:hover { border-color: #ff5fa2; }
QCheckBox::indicator:checked { background: #ff5fa2; border-color: #ff5fa2; image: url("@ICONOS@/check.png"); }

/* ---------- tablas ---------- */
QTableWidget { background: transparent; border: none; gridline-color: transparent; color: #ede7f2; }
QTableWidget::item { padding: 6px 8px; border-bottom: 1px solid #2a2532; }
QHeaderView { background: transparent; }
QHeaderView::section {
    background: transparent; color: #9b90a8; border: none; border-bottom: 1px solid #332d3c;
    padding: 8px 8px; font-size: 9pt; font-weight: 600;
}
QTableCornerButton::section { background: transparent; border: none; }

/* ---------- lienzo ---------- */
QGraphicsView { border: 1px solid #2a2532; border-radius: 14px; }

/* ---------- barras de desplazamiento finas ---------- */
QScrollBar:vertical { background: transparent; width: 10px; margin: 4px 2px 4px 0; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 0 4px 2px 4px; }
QScrollBar::handle:vertical { background: #3a3343; border-radius: 4px; min-height: 36px; }
QScrollBar::handle:horizontal { background: #3a3343; border-radius: 4px; min-width: 36px; }
QScrollBar::handle:hover { background: #ff5fa2; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; border: none; background: none; }
QScrollBar::add-page, QScrollBar::sub-page { background: none; }

/* ---------- menús, mensajes y ayudas ---------- */
QMenu { background: #25212c; border: 1px solid #332d3c; border-radius: 8px; padding: 6px; }
QMenu::item { padding: 7px 22px; border-radius: 6px; color: #ede7f2; }
QMenu::item:selected { background: #3b1a2d; color: #ff5fa2; }
QMenu::separator { height: 1px; background: #332d3c; margin: 5px 8px; }
QToolTip { background: #25212c; color: #ede7f2; border: 1px solid #ff5fa2; border-radius: 6px; padding: 6px; }
QMessageBox { background: #1b1821; }
QMessageBox QPushButton { min-width: 80px; }
)");
    css.replace(QStringLiteral("@ICONOS@"), iconos);
    return css;
}

} // namespace tema
