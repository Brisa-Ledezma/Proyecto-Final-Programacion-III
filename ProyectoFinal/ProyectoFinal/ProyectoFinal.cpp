// ============================================================
//  Proyecto Final - Programación III
//  Caminos en grafos: Dijkstra, Bellman-Ford y matriz de
//  adyacencia (Floyd-Warshall) con filtros y vista gráfica.
// ============================================================

#include "ui/Tema.h"
#include "ui/VentanaPrincipal.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication aplicacion(argc, argv);

    // Estilo claro fijo (igual al prototipo), aunque Windows esté en modo oscuro
    aplicacion.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QPalette paleta;
    paleta.setColor(QPalette::Window, tema::fondo());
    paleta.setColor(QPalette::WindowText, tema::tinta());
    paleta.setColor(QPalette::Base, tema::panel());
    paleta.setColor(QPalette::AlternateBase, tema::fondo());
    paleta.setColor(QPalette::Text, tema::tinta());
    paleta.setColor(QPalette::Button, tema::panel());
    paleta.setColor(QPalette::ButtonText, tema::tinta());
    paleta.setColor(QPalette::ToolTipBase, tema::panel());
    paleta.setColor(QPalette::ToolTipText, tema::tinta());
    paleta.setColor(QPalette::Highlight, tema::camino());
    paleta.setColor(QPalette::HighlightedText, tema::panel());
    paleta.setColor(QPalette::PlaceholderText, tema::apagado());
    aplicacion.setPalette(paleta);
    aplicacion.setStyleSheet(tema::hojaDeEstilo());

    VentanaPrincipal ventana;
    ventana.resize(1400, 860);
    ventana.show();
    return aplicacion.exec();
}
