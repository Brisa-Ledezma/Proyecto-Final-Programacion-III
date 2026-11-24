# Caminos en grafos — Proyecto Final de Programación III

Aplicación de escritorio en **C++17 + Qt 6** (Visual Studio 2022) que calcula el camino de menor costo entre dos nodos de un grafo y muestra el recorrido sobre el dibujo.

## Funcionalidades

- **Tres algoritmos:** Dijkstra, Bellman-Ford y matriz de adyacencia (Floyd-Warshall). La matriz de adyacencia se muestra siempre (hasta 150 nodos), con cualquier algoritmo.
- **Nodos numerados.** Cada nodo es un número desde 1 (1, 7, 23443 …, hasta 6 cifras). El campo solo deja tipear dígitos y no acepta el 0 ni números repetidos. Los nodos no tienen peso.
- **Grafo dirigido o no dirigido**, elegido para el grafo entero. No dirigido: cada arista tiene peso de ida y peso de vuelta. Dirigido: un peso por arista. Al pasar a dirigido, `A — B` queda solo como `A → B` con el peso de ida y se descarta el de vuelta; **Deshacer** vuelve al grafo no dirigido con sus pesos.
- **Pesos enteros.** Los campos de peso solo dejan tipear dígitos y un `-` al principio (ni punto, ni coma, ni `+`).
- **Hasta 150 nodos y 1000 aristas** (para que la aplicación siga fluida). Si no se escribe un número, se usa el menor libre.
- **Los nodos nunca se superponen.** Al crearlos o al soltarlos después de arrastrarlos se corren al lugar libre más cercano.
- **Las aristas no pasan por encima de otros nodos.** Se curvan para esquivarlos. El dibujo se arma en capas (aristas → camino → pesos → nodos) para que nada tape a un nodo.
- **Filtros:**
  - no pasar por un nodo específico (se bloquea el nodo; también se puede bloquear una arista puntual),
  - evitar las aristas de peso par o de peso impar.

  Los filtros de aristas cortan solo el sentido que tiene ese peso: en una no dirigida con ida 4 y vuelta 3, evitar pares corta la ida y deja la vuelta. Si el origen, el destino o un obligatorio quedan afuera, se muestra un error que dice por qué.
- **Nodos obligatorios:** se prueban todos los órdenes y se queda el más barato. Un nodo puede estar bloqueado u obligatorio, no las dos cosas.
- **Recorrido animado** paso a paso sobre el dibujo, más una tabla con el **detalle de cada paso** (peso de cada tramo y costo acumulado).
- **Tema oscuro con detalles rosas** y scroll de toda la página.
- **Deshacer** (o Ctrl + Z): revierte la última edición del grafo (hasta 100), **Cargar ejemplo** y **Limpiar todo**.
- **Quitar filtros**: apaga todos los filtros y desbloquea nodos y aristas. La línea «Filtros activos» muestra lo que está filtrado, y debajo del dibujo se ve cuántos nodos hay.
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
| Agregar nodo | Escribir el número (o dejarlo vacío) y tocar **Agregar**, o hacer **doble clic** en un lugar vacío del dibujo |
| Mover nodo | Arrastrarlo |
| Bloquear, marcar como obligatorio, usar como origen o destino, eliminar | Clic derecho sobre el nodo o la arista, o desde las tablas |
| Zoom | Ctrl + rueda del mouse |
| Deshacer la última edición | Botón «Deshacer» o Ctrl + Z |
| Agrandar / achicar la letra | Ctrl + (o Ctrl =) y Ctrl −; Ctrl 0 vuelve al tamaño inicial |
| Desplazar el dibujo | Arrastrar el fondo |
| Desplazar la página | Rueda del mouse |
| Cambiar entre dirigido y no dirigido | **Aristas → Tipo de grafo** |

El costo de pasar de `u` a `v` es el peso de la arista en ese sentido.

## Entrega (ejecutable)

1. Compilar en **Release | x64** (el `.exe` de Debug pide DLL de Debug que no se pueden redistribuir).
2. Copiar **toda** la carpeta `x64\Release` (el `.exe` más las DLL de Qt y la carpeta `platforms` que deja `windeployqt`), no solo el `.exe`.
3. En una PC sin Visual Studio, instalar el *Visual C++ Redistributable x64* o incluir sus DLL junto al `.exe`.
4. Probarlo con doble clic en una PC (o máquina virtual) limpia.

Si Qt no está en `C:\Qt\6.8.3\msvc2022_64`, cambiar la ruta en `RutaQt.props`. Si no se encuentra `windeployqt`, la compilación falla con un mensaje.

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
        │   └── Algoritmos.h/.cpp  Dijkstra, Bellman-Ford, matriz de adyacencia, filtros
        └── ui/                  interfaz Qt
            ├── Tema.h           tema oscuro con detalles rosas
            ├── Controles.h      campos de solo enteros que no cambian con la rueda sin foco
            ├── ItemsGrafo.*     nodo, arista y etiqueta del dibujo
            ├── LienzoGrafo.*    dibujo interactivo, sin superposiciones
            ├── PanelResultado.* costo, camino, tablas y matrices
            └── VentanaPrincipal.*  ventana con las tarjetas
```

## Ramas

- `main`: versiones estables.
- `develop`: integración.
- `feature/*`: una rama por funcionalidad, que se mergea a `develop`.
- `fix/*`, `refactor/*`, `docs/*`: correcciones, reorganizaciones y documentación, con el mismo circuito.
