// ============================================================
//  Proyecto Final - Programación III
//  Caminos en grafos: Dijkstra, Bellman-Ford y matriz de
//  adyacencia (Floyd-Warshall) con filtros y vista gráfica.
// ============================================================

#include "ui/Tema.h"
#include "ui/VentanaPrincipal.h"

#include <QApplication>
#include <QDir>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QStyleFactory>

namespace {

// Dibuja los íconos del tema (tilde de las casillas y flecha de los
// combos) y los guarda en una carpeta temporal para la hoja de estilo.
QString prepararIconos() {
    const QString carpeta = QDir::tempPath() + QStringLiteral("/caminos-en-grafos-iconos");
    QDir().mkpath(carpeta);

    QPixmap tilde(36, 36); // doble resolución: se ve nítido en pantallas con escala
    tilde.fill(Qt::transparent);
    {
        QPainter p(&tilde);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(QColor("#1a0f16"), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF puntos[3] = {QPointF(9, 19), QPointF(15, 25), QPointF(27, 11)};
        p.drawPolyline(puntos, 3);
    }
    tilde.save(carpeta + QStringLiteral("/check.png"));

    QPixmap flecha(20, 12);
    flecha.fill(Qt::transparent);
    {
        QPainter p(&flecha);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(tema::apagado(), 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF puntos[3] = {QPointF(3, 3), QPointF(10, 9), QPointF(17, 3)};
        p.drawPolyline(puntos, 3);
    }
    flecha.save(carpeta + QStringLiteral("/flecha.png"));
    return carpeta;
}

QPalette paletaOscura() {
    QPalette paleta;
    paleta.setColor(QPalette::Window, tema::fondo());
    paleta.setColor(QPalette::WindowText, tema::tinta());
    paleta.setColor(QPalette::Base, tema::campo());
    paleta.setColor(QPalette::AlternateBase, tema::panel());
    paleta.setColor(QPalette::Text, tema::tinta());
    paleta.setColor(QPalette::Button, tema::campo());
    paleta.setColor(QPalette::ButtonText, tema::tinta());
    paleta.setColor(QPalette::ToolTipBase, tema::campo());
    paleta.setColor(QPalette::ToolTipText, tema::tinta());
    paleta.setColor(QPalette::Highlight, tema::camino());
    paleta.setColor(QPalette::HighlightedText, tema::fondo());
    paleta.setColor(QPalette::PlaceholderText, tema::tenue());
    paleta.setColor(QPalette::Disabled, QPalette::Text, tema::tenue());
    paleta.setColor(QPalette::Disabled, QPalette::ButtonText, tema::tenue());
    paleta.setColor(QPalette::Disabled, QPalette::WindowText, tema::tenue());
    return paleta;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication aplicacion(argc, argv);

    // Tema oscuro fijo, independiente de la configuración de Windows
    aplicacion.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    aplicacion.setPalette(paletaOscura());
    aplicacion.setStyleSheet(tema::hojaDeEstilo(prepararIconos()));

    VentanaPrincipal ventana;
    ventana.resize(1440, 900);
    ventana.show();
    return aplicacion.exec();
}
