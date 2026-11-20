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
    bool evitarPares = false;   // excluye los nodos n.º 2, 4, 6 ... (posición del nodo, no su nombre)
    bool evitarImpares = false; // excluye los nodos 1, 3, 5 ...
    // Pesos bloqueados: no se usa ningún sentido de arista que pese esto.
    // En una no dirigida con ida 4 y vuelta 2, bloquear el 4 corta solo la ida.
    std::vector<int> pesosBloqueados;
    bool evitarAristasPares = false;   // corta los sentidos de arista con peso par (0, 2, -4 ...)
    bool evitarAristasImpares = false; // corta los sentidos de arista con peso impar (1, 3, -5 ...)
    // Signo del peso (excluyentes entre sí). El 0 no es ni positivo ni negativo: con cualquiera
    // de los dos filtros activos, un sentido de arista con peso 0 queda cortado.
    bool soloPositivos = false; // solo se usan sentidos de arista con peso > 0
    bool soloNegativos = false; // solo se usan sentidos de arista con peso < 0
};

// ¿Un sentido de arista con ese peso queda cortado (lista de pesos o paridad)?
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

    // Se arma con cualquier algoritmo
    std::vector<std::vector<double>> matrizAdyacencia;
};

const int MAXIMO_OBLIGATORIOS = 8;

// ¿El nodo queda afuera por bloqueo o por el filtro par/impar?
bool nodoExcluido(const Grafo& g, int indice, const Filtros& f);

// "" si el nodo puede usarse; si no, el motivo (empezando por `quien`).
std::string motivoExclusion(const Grafo& g, int indice, const Filtros& f, const std::string& quien);

Resultado resolver(const Grafo& g, int origen, int destino, Algoritmo alg, const Filtros& f);

std::string nombreAlgoritmo(Algoritmo alg);

} // namespace grafos
