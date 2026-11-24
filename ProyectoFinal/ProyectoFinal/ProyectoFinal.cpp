// ============================================================
//  Proyecto Final - Programación III
//  Caminos en grafos: Dijkstra, Bellman-Ford y matriz de
//  adyacencia con filtros y vista gráfica.
// ============================================================

#include "ui/Tema.h"
#include "ui/VentanaPrincipal.h"

#include <QApplication>
#include <QDir>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QScreen>
#include <QStyleFactory>

namespace {

// Prueba cada carpeta candidata (en orden) hasta que se pueden guardar los dos íconos.
QString guardarIconos(const QStringList& carpetas, int i, const QPixmap& tilde, const QPixmap& flecha) {
    if (i >= static_cast<int>(carpetas.size())) return carpetas.value(0);
    QDir().mkpath(carpetas[i]);
    if (tilde.save(carpetas[i] + QStringLiteral("/check.png")) && flecha.save(carpetas[i] + QStringLiteral("/flecha.png")))
        return carpetas[i];
    return guardarIconos(carpetas, i + 1, tilde, flecha);
}

// Dibuja los íconos del tema y los guarda en una carpeta para la hoja de estilo. Si no se puede escribir en la
// carpeta temporal, prueba con una carpeta oculta del usuario (sin los íconos, las casillas se verían sin tilde).
QString prepararIconos() {
    QPixmap tilde(36, 36); // doble resolución: se ve nítido en pantallas con escala
    tilde.fill(Qt::transparent);
    {
        QPainter p(&tilde);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(QColor("#1a0f16"), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF puntos[3] = {QPointF(9, 19), QPointF(15, 25), QPointF(27, 11)};
        p.drawPolyline(puntos, 3);
    }
    QPixmap flecha(20, 12);
    flecha.fill(Qt::transparent);
    {
        QPainter p(&flecha);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(tema::apagado(), 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF puntos[3] = {QPointF(3, 3), QPointF(10, 9), QPointF(17, 3)};
        p.drawPolyline(puntos, 3);
    }
    return guardarIconos({QDir::tempPath() + QStringLiteral("/caminos-en-grafos-iconos"),
                          QDir::homePath() + QStringLiteral("/.caminos-en-grafos-iconos")}, 0, tilde, flecha);
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

    // Tema oscuro
    aplicacion.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    aplicacion.setPalette(paletaOscura());
    const QString iconos = prepararIconos();
    aplicacion.setProperty("carpetaIconos", iconos);               // para rearmar la hoja de estilo al cambiar el zoom
    aplicacion.setProperty("escalaUI", tema::ESCALA_INICIAL);
    aplicacion.setStyleSheet(tema::hojaDeEstilo(iconos, tema::ESCALA_INICIAL));

    VentanaPrincipal ventana;
    // 1440 × 900 si entra; en pantallas más chicas (por ejemplo 1366 × 768) se ajusta al área disponible
    const QRect pantalla = aplicacion.primaryScreen()->availableGeometry();
    ventana.resize(qMin(1440, pantalla.width()), qMin(900, pantalla.height()));
    ventana.show();
    return aplicacion.exec();
}
