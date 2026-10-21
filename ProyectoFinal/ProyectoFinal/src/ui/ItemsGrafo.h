#pragma once

// ============================================================
//  ItemsGrafo.h
//  Elementos que se dibujan en el lienzo. Cada tipo vive en su
//  propia CAPA (valor Z), de abajo hacia arriba:
//      0  aristas comunes
//      1  aristas que forman parte del camino
//      2  etiquetas con el peso de las aristas
//      3  nodos
//  Así una línea nunca tapa un nodo ni el número de su peso.
// ============================================================

#include <QGraphicsItem>
#include <QPainterPath>
#include <QPolygonF>
#include <QString>
#include <functional>

namespace capas {
const double ARISTA = 0.0;
const double ARISTA_CAMINO = 1.0;
const double ETIQUETA = 2.0;
const double NODO = 3.0;
}

// Radio del círculo según el largo del nombre (mínimo 22 px).
double radioParaNombre(const QString& nombre);

// ------------------------------------------------------------
//  Nodo
// ------------------------------------------------------------
class NodoItem : public QGraphicsItem {
public:
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    NodoItem(int indice, const QString& nombre, int numero, double peso, double radio);

    int indice() const { return indice_; }
    double radio() const { return radio_; }

    // rol: "origen", "destino", "origen y destino" o ""
    void setEstado(bool bloqueado, bool obligatorio, bool filtrado, bool enCamino, const QString& rol);

    // Avisos hacia el lienzo
    std::function<void(int, QPointF)> alMoverse;
    std::function<void(int)> alSoltarse;

    // Activa el aviso de movimiento (se llama después del setPos inicial)
    void habilitarArrastre();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* p, const QStyleOptionGraphicsItem* opcion, QWidget* widget) override;

protected:
    QVariant itemChange(GraphicsItemChange cambio, const QVariant& valor) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* evento) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* evento) override;

private:
    int indice_;
    QString nombre_;
    int numero_;
    double peso_;
    double radio_;
    bool bloqueado_ = false;
    bool obligatorio_ = false;
    bool filtrado_ = false;
    bool enCamino_ = false;
    QString rol_;
};

// ------------------------------------------------------------
//  Arista (curva + flechas)
// ------------------------------------------------------------
class AristaItem : public QGraphicsItem {
public:
    enum { Type = UserType + 2 };
    int type() const override { return Type; }

    explicit AristaItem(int indice);
    int indice() const { return indice_; }

    void setGeometria(const QPainterPath& curva, const QPolygonF& flechaFinal, const QPolygonF& flechaInicial);
    // sentidoCamino: 0 = no está en el camino, 1 = se recorre desde->hasta, -1 = al revés
    void setEstado(bool dirigida, bool bloqueada, bool excluida, int sentidoCamino);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* p, const QStyleOptionGraphicsItem* opcion, QWidget* widget) override;

private:
    int indice_;
    QPainterPath curva_;
    QPolygonF flechaFinal_;
    QPolygonF flechaInicial_;
    bool dirigida_ = false;
    bool bloqueada_ = false;
    bool excluida_ = false;
    int sentido_ = 0;
};

// ------------------------------------------------------------
//  Etiqueta con el peso de una arista
// ------------------------------------------------------------
class EtiquetaItem : public QGraphicsItem {
public:
    enum { Type = UserType + 3 };
    int type() const override { return Type; }

    EtiquetaItem(int indiceArista, const QString& texto);
    int indiceArista() const { return indiceArista_; }
    void setEstado(bool bloqueada, bool excluida, bool enCamino);

    QRectF boundingRect() const override;
    void paint(QPainter* p, const QStyleOptionGraphicsItem* opcion, QWidget* widget) override;

private:
    int indiceArista_;
    QString texto_;
    QRectF caja_;
    bool bloqueada_ = false;
    bool excluida_ = false;
    bool enCamino_ = false;
};
