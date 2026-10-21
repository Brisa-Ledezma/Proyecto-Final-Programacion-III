# Caminos en grafos — Proyecto Final de Programación III

Aplicación de escritorio en **C++17 + Qt 6** (Visual Studio 2022) que calcula el camino de menor costo entre dos nodos de un grafo y muestra el recorrido sobre el dibujo.

## Funcionalidades

- **Tres algoritmos:** Dijkstra, Bellman-Ford y matriz de adyacencia (Floyd-Warshall). Con Floyd-Warshall se ven la matriz de adyacencia y la de distancias mínimas.
- **Nodos con nombre y peso entero.** El nombre solo admite letras, nunca números. El peso se suma cada vez que el camino entra al nodo.
- **Aristas dirigidas** (un solo peso) **y no dirigidas** (peso de ida y peso de vuelta). Todos los pesos son enteros.
- **Sin límite de nodos.** Se nombran solos (A … Z, AA, AB …) si no se escribe un nombre.
- **Los nodos nunca se superponen.** Al crearlos o al soltarlos después de arrastrarlos se corren al lugar libre más cercano.
- **Las aristas no pasan por encima de otros nodos.** Se curvan para esquivarlos. El dibujo se arma en capas (aristas → camino → pesos → nodos) para que nada tape a un nodo.
- **Filtros:**
  - bloquear una arista (evitar un camino específico),
  - bloquear un nodo,
  - evitar los nodos pares o los impares (según su número #1, #2, …). Si el origen o el destino quedan afuera por el filtro, se muestra un error.
- **Nodos obligatorios:** se prueban todos los órdenes y se queda el más barato.
- **Recorrido animado** paso a paso sobre el dibujo, más una tabla con el **detalle de cada paso** (peso de la arista, peso del nodo, costo y acumulado).
- **Tema oscuro con detalles rosas** y scroll de toda la página.
- **Cargar ejemplo** y **Limpiar todo**.
- **Sin `for` ni `while`:** todo el código usa solo `if` y recursión.

## Requisitos

1. **Visual Studio 2022** con la carga de trabajo *Desarrollo para el escritorio con C++*.
2. **Qt 6** (open source) con el kit **MSVC 2022 64-bit**, instalado con el [Qt Online Installer](https://www.qt.io/download-qt-installer-oss) en `C:\Qt`.

No hace falta la extensión *Qt VS Tools*: el proyecto no usa `Q_OBJECT` (las señales se conectan con lambdas), así que no necesita `moc`.

## Compilar y ejecutar

1. Si tu versión de Qt no es la que figura en `ProyectoFinal/ProyectoFinal/RutaQt.props`, cambiá la ruta en esa línea:
   ```xml
   <QtDir ...>C:\Qt\6.8.3\msvc2022_64</QtDir>
   ```
2. Abrí `ProyectoFinal/ProyectoFinal.sln`.
3. Elegí la configuración **Debug | x64** o **Release | x64**.
4. Ejecutá con **F5**. Al compilar, `windeployqt` copia las DLL de Qt junto al `.exe`.

## Uso

| Acción | Cómo |
|---|---|
| Agregar nodo | Escribir el nombre (o dejarlo vacío) y tocar **Agregar**, o hacer clic en un lugar vacío del dibujo |
| Mover nodo | Arrastrarlo |
| Bloquear, marcar como obligatorio, usar como origen o destino, eliminar | Clic derecho sobre el nodo o la arista, o desde las tablas |
| Zoom | Ctrl + rueda del mouse |
| Desplazar el dibujo | Arrastrar el fondo |
| Desplazar la página | Rueda del mouse |

El costo de pasar de `u` a `v` es el peso de la arista en ese sentido más el peso del nodo `v`.

## Estructura

```
ProyectoFinal/
├── ProyectoFinal.sln
└── ProyectoFinal/
    ├── ProyectoFinal.cpp        main(): arranca la aplicación Qt
    ├── RutaQt.props             ruta de instalación de Qt
    └── src/
        ├── core/                lógica en C++ puro (no depende de Qt)
        │   ├── Recorrer.h       reemplazo recursivo del for
        │   ├── Grafo.h/.cpp     nodos, aristas, validaciones, ejemplo
        │   └── Algoritmos.h/.cpp  Dijkstra, Bellman-Ford, Floyd-Warshall, filtros
        └── ui/                  interfaz Qt
            ├── Tema.h           tema oscuro con detalles rosas
            ├── Controles.h      campos que no cambian con la rueda sin foco
            ├── ItemsGrafo.*     nodo, arista y etiqueta del dibujo
            ├── LienzoGrafo.*    dibujo interactivo, sin superposiciones
            ├── PanelResultado.* costo, camino, tablas y matrices
            └── VentanaPrincipal.*  ventana con las tarjetas
```

## Ramas

- `main`: versiones estables.
- `develop`: integración.
- `feature/*`: una rama por funcionalidad, que se mergea a `develop`.
