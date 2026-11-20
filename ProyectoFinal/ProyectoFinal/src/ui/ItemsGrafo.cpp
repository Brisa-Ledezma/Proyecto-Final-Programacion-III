#include "ItemsGrafo.h"
#include "Tema.h"

#include <QCursor>
#include <QFont>
#include <QFontMetricsF>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPen>
#include <algorithm>

namespace {

QFont fuente(int pixeles, bool negrita) {
    QFont f("Segoe UI");
    f.setPixelSize(pixeles);
    f.setBold(negrita);
    return f;
}


} // namespace

double radioParaNombre(const QString& nombre) {
    const QFontMetricsF fm(fuente(15, true));
    return std::max(22.0, fm.horizontalAdvance(nombre) / 2.0 + 10.0);
}

// ============================================================
//  NodoItem
// ============================================================
NodoItem::NodoItem(int indice, const QString& nombre, double radio)
    : indice_(indice), nombre_(nombre), radio_(radio) {
    setZValue(capas::NODO);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setCursor(Qt::OpenHandCursor);
    setAcceptHoverEvents(true);
    setToolTip(QStringLiteral("Nodo %1\nArrastralo para moverlo · clic derecho para opciones").arg(nombre));
}

void NodoItem::habilitarArrastre() {
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

void NodoItem::setEstado(bool bloqueado, bool obligatorio, bool filtrado, bool enCamino, const QString& rol) {
    bloqueado_ = bloqueado;
    obligatorio_ = obligatorio;
    filtrado_ = filtrado;
    enCamino_ = enCamino;
    rol_ = rol;
    setOpacity(filtrado ? 0.35 : 1.0);
    update();
}

QRectF NodoItem::boundingRect() const {
    const double r = radio_;
    return QRectF(-r - 40, -r - 24, 2 * r + 80, 2 * r + 48);
}

QPainterPath NodoItem::shape() const {
    QPainterPath p;
    p.addEllipse(QPointF(0, 0), radio_ + 2, radio_ + 2);
    return p;
}

void NodoItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
    p->setRenderHint(QPainter::Antialiasing, true);
    const double r = radio_;

    // Color del borde según el estado (misma lógica que el prototipo)
    QColor borde = tema::tenue();
    double grosor = 2.0;
    if (enCamino_) { borde = tema::camino(); grosor = 3.0; }
    if (obligatorio_) { borde = tema::obligatorio(); grosor = 3.5; }
    if (bloqueado_) { borde = tema::bloqueado(); grosor = 3.5; }

    QPen pluma(borde, grosor);
    if (bloqueado_ || filtrado_) pluma.setStyle(Qt::DashLine);
    p->setPen(pluma);
    p->setBrush(enCamino_ ? tema::caminoFondo() : tema::campo());
    p->drawEllipse(QPointF(0, 0), r, r);

    // Nombre
    p->setFont(fuente(15, true));
    p->setPen(tema::tinta());
    p->drawText(QRectF(-r, -r, 2 * r, 2 * r), Qt::AlignCenter, nombre_);

    // Origen / destino
    if (!rol_.isEmpty()) {
        p->setFont(fuente(11, true));
        p->setPen(tema::camino());
        p->drawText(QRectF(-r - 40, -r - 22, 2 * r + 80, 16), Qt::AlignCenter, rol_);
    }
}

QVariant NodoItem::itemChange(GraphicsItemChange cambio, const QVariant& valor) {
    if (cambio == ItemPositionHasChanged && alMoverse) alMoverse(indice_, pos());
    return QGraphicsItem::itemChange(cambio, valor);
}

void NodoItem::mousePressEvent(QGraphicsSceneMouseEvent* evento) {
    if (evento->button() == Qt::LeftButton) setCursor(Qt::ClosedHandCursor);
    QGraphicsItem::mousePressEvent(evento);
}

void NodoItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* evento) {
    setCursor(Qt::OpenHandCursor);
    QGraphicsItem::mouseReleaseEvent(evento);
    if (alSoltarse) alSoltarse(indice_);
}

// ============================================================
//  AristaItem
// ============================================================
AristaItem::AristaItem(int indice) : indice_(indice) {
    setZValue(capas::ARISTA);
    setAcceptedMouseButtons(Qt::NoButton); // un clic sobre la arista no la "agarra"
}

void AristaItem::setGeometria(const QPainterPath& curva, const QPolygonF& flechaFinal,
                              const QPolygonF& flechaInicial) {
    prepareGeometryChange();
    curva_ = curva;
    flechaFinal_ = flechaFinal;
    flechaInicial_ = flechaInicial;
    update();
}

void AristaItem::setEstado(bool dirigida, bool bloqueada, bool excluida, int sentidoCamino) {
    dirigida_ = dirigida;
    bloqueada_ = bloqueada;
    excluida_ = excluida;
    sentido_ = sentidoCamino;
    setZValue(sentidoCamino != 0 ? capas::ARISTA_CAMINO : capas::ARISTA);
    setOpacity(excluida && !bloqueada ? 0.35 : 1.0);
    update();
}

QRectF AristaItem::boundingRect() const {
    return curva_.boundingRect()
        .united(flechaFinal_.boundingRect())
        .united(flechaInicial_.boundingRect())
        .adjusted(-8, -8, 8, 8);
}

QPainterPath AristaItem::shape() const {
    QPainterPathStroker trazo;
    trazo.setWidth(12);
    return trazo.createStroke(curva_);
}

void AristaItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
    p->setRenderHint(QPainter::Antialiasing, true);
    const bool enCamino = sentido_ != 0;

    QColor color = tema::tenue();
    if (enCamino) color = tema::camino();
    if (bloqueada_) color = tema::bloqueado();

    // Brillo suave detrás del camino
    if (enCamino) {
        QColor brillo = tema::camino();
        brillo.setAlpha(70);
        p->setPen(QPen(brillo, 10, Qt::SolidLine, Qt::RoundCap));
        p->setBrush(Qt::NoBrush);
        p->drawPath(curva_);
    }

    QPen pluma(color, enCamino ? 3.5 : 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    if (bloqueada_) pluma.setStyle(Qt::DashLine);
    p->setPen(pluma);
    p->setBrush(Qt::NoBrush);
    p->drawPath(curva_);

    // Flechas: siempre en las dirigidas; en las no dirigidas solo
    // cuando están en el camino, para mostrar hacia dónde se recorre.
    p->setPen(QPen(color, 1));
    p->setBrush(color);
    if (dirigida_ || sentido_ == 1) p->drawPolygon(flechaFinal_);
    if (!dirigida_ && sentido_ == -1) p->drawPolygon(flechaInicial_);
}

// ============================================================
//  EtiquetaItem
// ============================================================
EtiquetaItem::EtiquetaItem(int indiceArista, const QString& texto)
    : indiceArista_(indiceArista), texto_(texto) {
    setZValue(capas::ETIQUETA);
    setAcceptedMouseButtons(Qt::NoButton);
    const QFontMetricsF fm(fuente(12, true)); // se mide en negrita: entra en ambos estados
    const double ancho = fm.horizontalAdvance(texto) + 12;
    const double alto = fm.height() + 4;
    caja_ = QRectF(-ancho / 2, -alto / 2, ancho, alto);
}

void EtiquetaItem::setEstado(bool bloqueada, bool excluida, bool enCamino) {
    bloqueada_ = bloqueada;
    excluida_ = excluida;
    enCamino_ = enCamino;
    setOpacity(excluida && !bloqueada ? 0.35 : 1.0);
    update();
}

QRectF EtiquetaItem::boundingRect() const { return caja_.adjusted(-2, -2, 2, 2); }

void EtiquetaItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
    p->setRenderHint(QPainter::Antialiasing, true);
    QColor color = tema::apagado();
    if (enCamino_) color = tema::camino();
    if (bloqueada_) color = tema::bloqueado();

    p->setPen(QPen(enCamino_ ? tema::camino() : tema::linea(), 1));
    p->setBrush(enCamino_ ? tema::caminoFondo() : tema::fondo());
    p->drawRoundedRect(caja_, 7, 7);

    QFont f = fuente(12, enCamino_);
    p->setFont(f);
    p->setPen(color);
    p->drawText(caja_, Qt::AlignCenter, texto_);
}
