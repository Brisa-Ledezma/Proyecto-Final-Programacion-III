#pragma once

// ============================================================
//  PanelResultado.h
//  Tarjetas de resultado debajo del dibujo:
//   - Resultado: costo total y camino
//   - Detalle del recorrido: cada paso con su costo
//   - Matriz de adyacencia
// ============================================================

#include "core/Algoritmos.h"
#include "core/Grafo.h"

#include <QWidget>

class QVBoxLayout;

class PanelResultado : public QWidget {
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
