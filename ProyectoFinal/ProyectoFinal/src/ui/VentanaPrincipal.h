#pragma once

// ============================================================
//  VentanaPrincipal.h
//  Página con scroll general y dos columnas:
//  izquierda -> tarjetas Nodos, Aristas, Filtros y Resolver
//  derecha   -> dibujo del grafo y tarjetas de resultado
// ============================================================

#include "core/Algoritmos.h"
#include "core/Grafo.h"

#include <QMainWindow>
#include <QPointF>
#include <vector>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QFrame;
class QLabel;
class QLineEdit;
class QTableWidget;
class QVBoxLayout;
class QWidget;
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
    QTableWidget* tablaNodos_ = nullptr;
    // Aristas
    QComboBox* desde_ = nullptr;
    QComboBox* hasta_ = nullptr;
    QComboBox* tipoArista_ = nullptr;
    QSpinBox* pesoIda_ = nullptr;
    QSpinBox* pesoVuelta_ = nullptr;
    QLabel* etiquetaIda_ = nullptr;
    QLabel* etiquetaVuelta_ = nullptr;
    QTableWidget* tablaAristas_ = nullptr;
    // Filtros
    QCheckBox* evitarPares_ = nullptr;
    QCheckBox* evitarImpares_ = nullptr;
    QSpinBox* pesoABloquear_ = nullptr;
    QWidget* listaPesos_ = nullptr;          // un botón por peso bloqueado
    std::vector<int> pesosBloqueados_;
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
    QFrame* tarjetaLienzo();

    // ---- refresco ----
    void refrescarTodo();
    void refrescarCombos();
    void refrescarTablaNodos();
    void refrescarTablaAristas();
    void cambio();
    void refrescarPesosBloqueados();
    void agregarPesoBloqueado();
    void quitarPesoBloqueado(int peso);
    void limpiarFiltros();

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
