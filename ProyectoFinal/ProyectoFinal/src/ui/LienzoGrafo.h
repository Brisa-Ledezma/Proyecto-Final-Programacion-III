#pragma once

// ============================================================
//  LienzoGrafo.h
//  Dibujo interactivo del grafo (QGraphicsView).
//   - Clic en un lugar vacío: agrega un nodo ahí.
//   - Arrastrar un nodo: lo mueve (al soltarlo, si quedó encima
//     de otro, se corre al lugar libre más cercano).
//   - Arrastrar el fondo: desplaza la vista. Ctrl + rueda: zoom.
//   - Clic derecho sobre nodo o arista: menú de opciones.
//  Las aristas se curvan para no pasar por encima de otros nodos.
// ============================================================

#include "core/Algoritmos.h"
#include "core/Grafo.h"

#include <QGraphicsView>
#include <QPoint>
#include <QPointF>
#include <functional>
#include <vector>

class QGraphicsScene;
class QTimer;
class NodoItem;
class AristaItem;
class EtiquetaItem;

class LienzoGrafo : public QGraphicsView {
public:
    explicit LienzoGrafo(grafos::Grafo* grafo, QWidget* parent = nullptr);

    // ---- avisos hacia la ventana principal ----
    std::function<void(QPointF)> alHacerClicEnVacio;
    std::function<void(int, QPoint)> alPedirMenuNodo;
    std::function<void(int, QPoint)> alPedirMenuArista;

    // ---- estado que muestra ----
    void setFiltros(const grafos::Filtros& filtros);
    void setOrigenDestino(int origen, int destino);
    // nullptr = sin camino. Si animar es true, el recorrido se pinta paso a paso.
    void setResultado(const grafos::Resultado* resultado, bool animar);
    void repetirAnimacion();

    // Vuelve a crear todos los elementos a partir del grafo.
    void reconstruir();
    void ajustarVista();
    // Agranda o achica todo el dibujo (nodos, aristas y números) multiplicando el zoom actual.
    void escalarVista(double factor);

    // Posición sin superposición más cercana a "deseada".
    QPointF posicionLibre(QPointF deseada, double radio, int ignorar) const;
    QPointF centroVisible() const;

protected:
    void mousePressEvent(QMouseEvent* evento) override;
    void mouseReleaseEvent(QMouseEvent* evento) override;
    void wheelEvent(QWheelEvent* evento) override;
    void contextMenuEvent(QContextMenuEvent* evento) override;

private:
    grafos::Grafo* grafo_;
    QGraphicsScene* escena_;
    QTimer* temporizador_;

    std::vector<NodoItem*> nodos_;
    std::vector<AristaItem*> aristas_;
    std::vector<EtiquetaItem*> etiquetas_;

    grafos::Filtros filtros_;
    int origen_ = -1;
    int destino_ = -1;
    std::vector<int> camino_;
    std::vector<int> aristasCamino_;
    int pasosVisibles_ = 0;

    QPoint puntoPresionado_;
    bool presionEnVacio_ = false;

    void nodoMoviendose(int indice, QPointF posicion);
    void nodoSoltado(int indice);
    void rutearAristas();
    void rutearArista(int k);
    void rutearAristasDe(int nodo);
    void aplicarEstados();
    void actualizarRectEscena();
    void avanzarAnimacion();

    // auxiliares de posicionamiento
    bool lugarLibre(QPointF p, double radio, int ignorar, int j) const;
    QPointF buscarEnEspiral(QPointF centro, double radio, int ignorar, int intento) const;
    double xMaxima(int j, double acumulado) const;
    bool curvaLibre(QPointF a, QPointF control, QPointF b, int excluirA, int excluirB, int muestra) const;
    bool puntoLejosDeNodos(QPointF p, int excluirA, int excluirB, int j) const;
    double buscarDesvio(QPointF a, QPointF b, QPointF normal, double base, int excluirA, int excluirB, int intento) const;
    void posicionEnGrupo(int k, int j, int& posicion, int& tamanio) const;
};
