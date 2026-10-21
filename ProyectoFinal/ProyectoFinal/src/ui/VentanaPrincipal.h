#pragma once

// ============================================================
//  VentanaPrincipal.h
//  Ventana con el mismo esquema que el prototipo HTML:
//  izquierda -> tarjetas Nodos, Aristas, Filtros y Resolver
//  derecha   -> dibujo del grafo y tarjeta de resultado
// ============================================================

#include "core/Algoritmos.h"
#include "core/Grafo.h"

#include <QMainWindow>
#include <QPointF>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFrame;
class QLabel;
class QLineEdit;
class QTableWidget;
class QVBoxLayout;
class LienzoGrafo;
class PanelResultado;

class VentanaPrincipal : public QMainWindow {
public:
    explicit VentanaPrincipal(QWidget* parent = nullptr);

private:
    grafos::Grafo grafo_;
    bool calculado_ = false; // después del primer cálculo, cada cambio recalcula

    // Nodos
    QLineEdit* nombreNodo_ = nullptr;
    QDoubleSpinBox* pesoNodo_ = nullptr;
    QTableWidget* tablaNodos_ = nullptr;
    // Aristas
    QComboBox* desde_ = nullptr;
    QComboBox* hasta_ = nullptr;
    QComboBox* tipoArista_ = nullptr;
    QDoubleSpinBox* pesoIda_ = nullptr;
    QDoubleSpinBox* pesoVuelta_ = nullptr;
    QLabel* etiquetaIda_ = nullptr;
    QLabel* etiquetaVuelta_ = nullptr;
    QTableWidget* tablaAristas_ = nullptr;
    // Filtros
    QCheckBox* evitarPares_ = nullptr;
    QCheckBox* evitarImpares_ = nullptr;
    // Resolver
    QComboBox* origen_ = nullptr;
    QComboBox* destino_ = nullptr;
    QComboBox* algoritmo_ = nullptr;
    QCheckBox* animar_ = nullptr;
    // Derecha
    LienzoGrafo* lienzo_ = nullptr;
    PanelResultado* resultado_ = nullptr;

    // ---- armado de la interfaz ----
    void construirInterfaz();
    QFrame* tarjeta(const QString& titulo, QVBoxLayout*& contenido);
    QFrame* tarjetaNodos();
    QFrame* tarjetaAristas();
    QFrame* tarjetaFiltros();
    QFrame* tarjetaResolver();

    // ---- refresco ----
    void refrescarTodo();
    void refrescarCombos();
    void refrescarTablaNodos();
    void refrescarTablaAristas();
    void cambio();

    // ---- acciones ----
    void agregarNodo(QPointF posicion, bool posicionElegida);
    void agregarArista();
    void eliminarNodo(int indice);
    void eliminarArista(int indice);
    void calcular(bool animar);
    void cargarEjemplo();
    void limpiarTodo();
    void menuNodo(int indice, QPoint posicionPantalla);
    void menuArista(int indice, QPoint posicionPantalla);

    grafos::Filtros filtros() const;
    grafos::Algoritmo algoritmoElegido() const;
};
