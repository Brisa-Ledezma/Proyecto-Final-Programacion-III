# Caminos en grafos — Proyecto Final de Programación III

Aplicación de escritorio en **C++17 + Qt 6** (Visual Studio 2022) que calcula el camino de menor costo entre dos nodos de un grafo y muestra el recorrido sobre el dibujo.

## Funcionalidades

- **Tres algoritmos:** Dijkstra, Bellman-Ford y matriz de adyacencia. La matriz de adyacencia se muestra siempre, con cualquier algoritmo.
- **Nodos numerados.** Cada nodo es un número desde 1 (1, 7, 23443 …, hasta 6 cifras). El campo solo deja tipear dígitos y no acepta el 0 ni números repetidos. Los nodos no tienen peso.
- **Grafo dirigido o no dirigido**, elegido para el grafo entero. No dirigido: cada arista tiene peso de ida y peso de vuelta. Dirigido: un peso por arista. Al cambiar el tipo, `A — B` pasa a `A → B` y `B → A`, y al volver se juntan de nuevo, sin perder pesos.
- **Pesos enteros.** Los campos de peso solo dejan tipear dígitos y un `-` al principio (ni punto, ni coma, ni `+`).
- **Sin límite de nodos.** Si no se escribe un número, se usa el menor libre.
- **Los nodos nunca se superponen.** Al crearlos o al soltarlos después de arrastrarlos se corren al lugar libre más cercano.
- **Las aristas no pasan por encima de otros nodos.** Se curvan para esquivarlos. El dibujo se arma en capas (aristas → camino → pesos → nodos) para que nada tape a un nodo.
- **Filtros:**
  - bloquear una arista puntual o un nodo,
  - evitar los nodos pares o los impares (según su número de nodo, es decir su posición: 1.º, 2.º, 3.º…, no su nombre),
  - evitar las aristas de peso par o de peso impar,
  - usar solo aristas de peso positivo o solo de peso negativo (son excluyentes y el 0 no cuenta como ninguno de los dos),
  - bloquear uno o varios pesos de arista (por ejemplo, el 4 corta todas las aristas que pesan 4).

  Los filtros de aristas cortan solo el sentido que tiene ese peso: en una no dirigida con ida 4 y vuelta 3, bloquear el 4 corta la ida y deja la vuelta. Si el origen, el destino o un obligatorio quedan afuera, se muestra un error que dice por qué.
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
| Agregar nodo | Escribir el número (o dejarlo vacío) y tocar **Agregar**, o hacer clic en un lugar vacío del dibujo |
| Mover nodo | Arrastrarlo |
| Bloquear, marcar como obligatorio, usar como origen o destino, eliminar | Clic derecho sobre el nodo o la arista, o desde las tablas |
| Zoom | Ctrl + rueda del mouse |
| Deshacer la última edición | Botón «Deshacer» o Ctrl + Z |
| Agrandar / achicar la letra | Ctrl + (o Ctrl =) y Ctrl −; Ctrl 0 vuelve al tamaño inicial |
| Desplazar el dibujo | Arrastrar el fondo |
| Desplazar la página | Rueda del mouse |
| Bloquear un peso de arista | Escribirlo en **Filtros → Bloquear aristas de peso** y tocar **Bloquear peso**; se quita con su ✕ |
| Cambiar entre dirigido y no dirigido | **Aristas → Tipo de grafo** |

El costo de pasar de `u` a `v` es el peso de la arista en ese sentido.

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
