#pragma once

// ============================================================
//  PanelResultado.h
//  Tarjeta de resultado: costo total, camino, orden de los
//  obligatorios, costos desde el origen y (con Floyd-Warshall)
//  la matriz de adyacencia y la de distancias mínimas.
// ============================================================

#include "core/Algoritmos.h"
#include "core/Grafo.h"

#include <QFrame>

class QVBoxLayout;
class QWidget;

class PanelResultado : public QFrame {
public:
    explicit PanelResultado(QWidget* parent = nullptr);

    void mostrarVacio();
    void mostrarError(const QString& mensaje);
    void mostrar(const grafos::Grafo& grafo, const grafos::Resultado& resultado);

private:
    QVBoxLayout* disposicion_;
    QWidget* contenido_ = nullptr;

    QVBoxLayout* nuevoContenido();
};
