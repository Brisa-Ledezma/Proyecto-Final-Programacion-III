#pragma once

// ============================================================
//  Algoritmos.h
//  Camino mínimo con Dijkstra, Bellman-Ford o la matriz de adyacencia
//  (matriz de adyacencia), aplicando filtros (bloqueos,
//  nodos pares/impares, pesos de arista) y nodos obligatorios.
//  Todo implementado con if + recursión, sin for ni while.
// ============================================================

#include "Grafo.h"

#include <limits>
#include <string>
#include <vector>

namespace grafos {

const double INFINITO = std::numeric_limits<double>::infinity();

enum class Algoritmo { Dijkstra, BellmanFord, MatrizAdyacencia };

struct Filtros {
    // Aristas por paridad del peso: cortan cada sentido de arista que pesa par (0, 2, -4 ...) o impar (1, 3, -5 ...).
    // En una no dirigida con ida 4 y vuelta 3, evitar pares corta solo la ida.
    bool evitarAristasPares = false;
    bool evitarAristasImpares = false;
    // Un nodo específico se evita bloqueándolo (Nodo::bloqueado), no con un filtro de este struct.
};

// ¿Un sentido de arista con ese peso queda cortado por la paridad?
bool pesoBloqueado(const Filtros& f, int peso);

// Arco = una dirección concreta de una arista (u -> v) ya con su costo.
struct Arco {
    int hacia = -1;
    double costo = 0.0;
    int arista = -1; // índice de la arista de la que sale
};

struct Resultado {
    bool ok = false;
    std::string error;
    Algoritmo algoritmo = Algoritmo::Dijkstra;
    int origen = -1;
    int destino = -1;

    double costo = 0.0;
    std::vector<int> camino;          // índices de nodos, en orden
    std::vector<int> aristasCamino;   // arista usada en cada paso (tamaño camino-1)
    std::vector<int> ordenObligatorios;
    std::vector<double> distDesdeOrigen;
    std::vector<bool> excluido;       // nodos sacados por bloqueo o filtro
    int sentidosPorPeso = 0;          // sentidos de arista cortados por un peso bloqueado
    std::string aviso;                // algo que conviene saber aunque el camino se haya encontrado

    // Se arma con cualquier algoritmo
    std::vector<std::vector<double>> matrizAdyacencia;
};

const int MAXIMO_OBLIGATORIOS = 8;

// La matriz de adyacencia se arma con cualquier algoritmo mientras el grafo no pase del máximo de nodos
// (con el máximo son 150 × 150 celdas).
const int MAXIMO_NODOS_EN_MATRIZ = Grafo::MAXIMO_NODOS; // la matriz se muestra siempre
// La matriz de adyacencia como algoritmo es cúbica (n³): más allá de esto se pide otro algoritmo.
const int MAXIMO_NODOS_ALGORITMO_MATRIZ = 300;

// ¿El nodo queda afuera porque está bloqueado?
bool nodoExcluido(const Grafo& g, int indice, const Filtros& f);

// "" si el nodo puede usarse; si no, el motivo (empezando por `quien`).
std::string motivoExclusion(const Grafo& g, int indice, const Filtros& f, const std::string& quien);

Resultado resolver(const Grafo& g, int origen, int destino, Algoritmo alg, const Filtros& f);

std::string nombreAlgoritmo(Algoritmo alg);

} // namespace grafos
