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
class QPushButton;
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
    QCheckBox* evitarAristasPares_ = nullptr;
    QCheckBox* evitarAristasImpares_ = nullptr;
    QCheckBox* soloPositivos_ = nullptr;
    QCheckBox* soloNegativos_ = nullptr;
    QSpinBox* pesoABloquear_ = nullptr;
    QWidget* listaPesos_ = nullptr;          // un botón por peso bloqueado
    std::vector<int> pesosBloqueados_;
    // Resolver
    QComboBox* origen_ = nullptr;
    QComboBox* destino_ = nullptr;
    QComboBox* algoritmo_ = nullptr;
    QCheckBox* animar_ = nullptr;
    QWidget* izquierda_ = nullptr; // columna izquierda: su ancho acompaña a la letra
    QLabel* filtrosActivos_ = nullptr;  // resumen de lo que está filtrado
    QPushButton* quitarFiltros_ = nullptr;
    QPushButton* deshacer_ = nullptr;
    QLabel* contadorNodos_ = nullptr;   // "N nodos en el lienzo"
    // Deshacer: cada edición del grafo guarda cómo estaba antes
    struct Estado {
        grafos::Grafo grafo;
        int origen = 0;
        int destino = 0;
    };
    std::vector<Estado> historial_;
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
    void sincronizarTipo();
    void refrescarPesosBloqueados();
    void agregarPesoBloqueado();
    void quitarPesoBloqueado(int peso);
    void limpiarFiltros();
    void quitarFiltros();          // botón "Quitar filtros": filtros, pesos, nodos y aristas bloqueados
    void actualizarEstado();       // contador de nodos y línea "Filtros activos"
    void guardarParaDeshacer();    // llamar justo ANTES de modificar el grafo
    void deshacer();

    // ---- zoom de la letra (Ctrl + / Ctrl - / Ctrl 0) ----
    void crearAtajosDeZoom();
    void cambiarEscala(double nueva);

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
