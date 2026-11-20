#include "PanelResultado.h"
#include "Tema.h"
#include "core/Recorrer.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

using grafos::recorrer;

namespace {

const int MAXIMO_NODOS_EN_MATRIZ = 60; // solo limita lo que se MUESTRA, no el grafo
const int CHIPS_POR_FILA = 7;

QString nombreDe(const grafos::Grafo& g, int i) { return QString::fromStdString(g.nodo(i).nombre); }

QString formato(double v) {
    if (!(v < grafos::INFINITO)) return QStringLiteral("∞"); // ∞
    return QString::number(v, 'g', 10);
}

QLabel* etiqueta(const QString& texto, const char* nombreObjeto) {
    auto* l = new QLabel(texto);
    l->setObjectName(QString::fromLatin1(nombreObjeto));
    l->setWordWrap(true);
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return l;
}

// Tarjeta con título (y subtítulo opcional). Devuelve el layout interno.
QVBoxLayout* agregarTarjeta(QVBoxLayout* destino, const QString& titulo, const QString& subtitulo) {
    auto* marco = new QFrame;
    marco->setObjectName(QStringLiteral("tarjeta"));
    auto* capa = new QVBoxLayout(marco);
    capa->setContentsMargins(22, 18, 22, 20);
    capa->setSpacing(10);
    if (!titulo.isEmpty()) capa->addWidget(etiqueta(titulo, "tituloTarjeta"));
    if (!subtitulo.isEmpty()) capa->addWidget(etiqueta(subtitulo, "pista"));
    destino->addWidget(marco);
    return capa;
}

QTableWidget* tablaCompacta(int filas, int columnas) {
    auto* t = new QTableWidget(filas, columnas);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->setFocusPolicy(Qt::NoFocus);
    t->setShowGrid(false);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setMinimumSectionSize(54);
    t->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    t->verticalHeader()->setDefaultSectionSize(36);
    t->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    t->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    return t;
}

void celda(QTableWidget* t, int fila, int columna, const QString& texto, bool resaltar, bool centrar) {
    auto* item = new QTableWidgetItem(texto);
    item->setTextAlignment(centrar ? Qt::Alignment(Qt::AlignCenter) : (Qt::AlignVCenter | Qt::AlignLeft));
    if (resaltar) {
        item->setBackground(tema::caminoFondo());
        item->setForeground(tema::camino());
    }
    t->setItem(fila, columna, item);
}

// Alto justo para mostrar todas las filas: la página entera es la que hace scroll.
void ajustarAlto(QTableWidget* t) {
    int alto = t->horizontalHeader()->isVisible() ? t->horizontalHeader()->sizeHint().height() : 0;
    alto += t->verticalHeader()->defaultSectionSize() * t->rowCount();
    t->setFixedHeight(alto + 2 * t->frameWidth() + 14); // + lugar para la barra horizontal
}

QTableWidget* tablaMatriz(const grafos::Grafo& g, const std::vector<std::vector<double>>& m,
                          const std::vector<int>& camino) {
    const int n = static_cast<int>(m.size());
    auto* t = tablaCompacta(n, n);
    QStringList encabezados;
    recorrer(0, n, [&](int i) { encabezados << nombreDe(g, i); });
    t->setHorizontalHeaderLabels(encabezados);
    t->setVerticalHeaderLabels(encabezados);

    std::vector<std::vector<bool>> enCamino(n, std::vector<bool>(n, false));
    recorrer(0, static_cast<int>(camino.size()) - 1, [&](int i) { enCamino[camino[i]][camino[i + 1]] = true; });

    recorrer(0, n, [&](int i) {
        recorrer(0, n, [&](int j) { celda(t, i, j, formato(m[i][j]), enCamino[i][j], true); });
    });
    ajustarAlto(t);
    return t;
}

// Camino como "chips" en filas de CHIPS_POR_FILA: A → B → C ...
QWidget* chipsDelCamino(const grafos::Grafo& g, const std::vector<int>& camino) {
    auto* contenedor = new QWidget;
    auto* grilla = new QGridLayout(contenedor);
    grilla->setContentsMargins(0, 4, 0, 4);
    grilla->setHorizontalSpacing(8);
    grilla->setVerticalSpacing(10);
    const int total = static_cast<int>(camino.size());
    recorrer(0, total, [&](int i) {
        const int fila = i / CHIPS_POR_FILA;
        const int columna = (i % CHIPS_POR_FILA) * 2;
        QLabel* chip = etiqueta(nombreDe(g, camino[i]), "chip");
        chip->setWordWrap(false);
        grilla->addWidget(chip, fila, columna);
        if (i + 1 < total) {
            QLabel* flecha = etiqueta(QStringLiteral("→"), "flechaChip");
            flecha->setWordWrap(false);
            grilla->addWidget(flecha, fila, columna + 1);
        }
    });
    grilla->setColumnStretch(CHIPS_POR_FILA * 2, 1);
    return contenedor;
}

// Una fila por paso: tramo, peso de la arista y acumulado.
QTableWidget* tablaDetalle(const grafos::Grafo& g, const grafos::Resultado& r) {
    const int pasos = static_cast<int>(r.camino.size()) - 1;
    auto* t = tablaCompacta(pasos, 5);
    t->setHorizontalHeaderLabels({QStringLiteral("PASO"), QStringLiteral("TRAMO"), QStringLiteral("PESO"),
                                  QStringLiteral("ACUMULADO"), QStringLiteral("NOTA")});
    t->verticalHeader()->hide();
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    int acumulado = 0;
    recorrer(0, pasos, [&](int i) {
        const int u = r.camino[i];
        const int v = r.camino[i + 1];
        const int k = r.aristasCamino[i];
        const grafos::Arista& e = g.arista(k);
        const int pesoArista = (e.desde == u) ? e.pesoIda : e.pesoVuelta;
        acumulado += pesoArista;

        QString nota;
        if (v == r.destino && i == pasos - 1) nota = QStringLiteral("llegada al destino");
        else if (g.nodo(v).obligatorio) nota = QStringLiteral("pasa por un obligatorio");

        celda(t, i, 0, QString::number(i + 1), false, true);
        celda(t, i, 1, nombreDe(g, u) + QStringLiteral("  →  ") + nombreDe(g, v), false, false);
        celda(t, i, 2, QString::number(pesoArista), false, true);
        celda(t, i, 3, QString::number(acumulado), true, true);
        celda(t, i, 4, nota, false, false);
    });
    ajustarAlto(t);
    return t;
}

} // namespace

// ============================================================
PanelResultado::PanelResultado(QWidget* parent) : QWidget(parent) {
    disposicion_ = new QVBoxLayout(this);
    disposicion_->setContentsMargins(0, 0, 0, 0);
    mostrarVacio();
}

QVBoxLayout* PanelResultado::nuevoContenido() {
    delete contenido_; // borra el contenido anterior con todos sus hijos
    contenido_ = new QWidget(this);
    auto* capa = new QVBoxLayout(contenido_);
    capa->setContentsMargins(0, 0, 0, 0);
    capa->setSpacing(16);
    disposicion_->addWidget(contenido_);
    return capa;
}

void PanelResultado::mostrarVacio() {
    auto* tarjeta = agregarTarjeta(nuevoContenido(), QStringLiteral("Resultado"), QString());
    tarjeta->addWidget(etiqueta(QStringLiteral("Agregá nodos y aristas, o cargá el ejemplo, y tocá «Calcular camino»."), "pista"));
}

void PanelResultado::mostrarError(const QString& mensaje) {
    auto* tarjeta = agregarTarjeta(nuevoContenido(), QStringLiteral("Resultado"), QString());
    tarjeta->addWidget(etiqueta(mensaje, "error"));
}

void PanelResultado::mostrar(const grafos::Grafo& g, const grafos::Resultado& r) {
    if (!r.ok) {
        mostrarError(QString::fromStdString(r.error));
        return;
    }
    QVBoxLayout* capa = nuevoContenido();
    const QString algoritmo = QString::fromStdString(grafos::nombreAlgoritmo(r.algoritmo));
    const int pasos = static_cast<int>(r.camino.size()) - 1;

    // ---------- Resultado ----------
    QVBoxLayout* resumen = agregarTarjeta(capa, QStringLiteral("Resultado"), QString());
    auto* filaCosto = new QHBoxLayout;
    filaCosto->setSpacing(14);
    filaCosto->addWidget(etiqueta(formato(r.costo), "costo"));
    auto* datos = new QVBoxLayout;
    datos->setSpacing(2);
    datos->addStretch(1);
    datos->addWidget(etiqueta(QStringLiteral("costo total"), "rotulo"));
    datos->addWidget(etiqueta(algoritmo + QStringLiteral(" · ") + QString::number(pasos) +
                                  (pasos == 1 ? QStringLiteral(" paso") : QStringLiteral(" pasos")), "pista"));
    filaCosto->addLayout(datos);
    filaCosto->addStretch(1);
    resumen->addLayout(filaCosto);
    resumen->addWidget(etiqueta(QStringLiteral("CAMINO"), "rotulo"));
    resumen->addWidget(chipsDelCamino(g, r.camino));
    if (!r.ordenObligatorios.empty()) {
        QStringList orden;
        recorrer(0, static_cast<int>(r.ordenObligatorios.size()),
                 [&](int i) { orden << nombreDe(g, r.ordenObligatorios[i]); });
        resumen->addWidget(etiqueta(QStringLiteral("Orden de paso por los obligatorios: ") + orden.join(QStringLiteral(", ")), "pista"));
    }

    // ---------- Detalle del recorrido ----------
    if (pasos > 0) {
        QVBoxLayout* detalle = agregarTarjeta(capa, QStringLiteral("Detalle del recorrido"),
            QStringLiteral("Costo de cada paso = peso de la arista en el sentido en que se recorre."));
        detalle->addWidget(tablaDetalle(g, r));
    } else {
        QVBoxLayout* detalle = agregarTarjeta(capa, QStringLiteral("Detalle del recorrido"), QString());
        detalle->addWidget(etiqueta(QStringLiteral("El origen y el destino son el mismo nodo: no hay pasos."), "pista"));
    }

    // ---------- Costo mínimo desde el origen ----------
    const int n = g.cantidadNodos();
    QVBoxLayout* distancias = agregarTarjeta(capa, QStringLiteral("Costo mínimo desde ") + nombreDe(g, r.origen),
        QStringLiteral("— = nodo bloqueado o filtrado  ·  ∞ = inalcanzable"));
    auto* tabla = tablaCompacta(1, n);
    QStringList encabezados;
    recorrer(0, n, [&](int i) {
        encabezados << nombreDe(g, i);
        const bool fuera = i < static_cast<int>(r.excluido.size()) && r.excluido[i];
        celda(tabla, 0, i, fuera ? QStringLiteral("—") : formato(r.distDesdeOrigen[i]), i == r.destino, true);
    });
    tabla->setHorizontalHeaderLabels(encabezados);
    tabla->verticalHeader()->hide();
    ajustarAlto(tabla);
    distancias->addWidget(tabla);

    // ---------- Matrices (solo Floyd-Warshall) ----------
    if (r.algoritmo == grafos::Algoritmo::Floyd) {
        if (n > MAXIMO_NODOS_EN_MATRIZ) {
            QVBoxLayout* m = agregarTarjeta(capa, QStringLiteral("Matrices"), QString());
            m->addWidget(etiqueta(QStringLiteral("Las matrices tienen %1 × %1 celdas: son demasiado grandes para "
                                                 "mostrarlas, pero el cálculo se hizo igual.").arg(n), "pista"));
        } else {
            QVBoxLayout* a = agregarTarjeta(capa, QStringLiteral("Matriz de adyacencia"),
                QStringLiteral("Peso del arco directo fila → columna. "
                               "En rosa, los arcos que usa el camino."));
            a->addWidget(tablaMatriz(g, r.matrizAdyacencia, r.camino));
            QVBoxLayout* d = agregarTarjeta(capa, QStringLiteral("Matriz de distancias mínimas"),
                QStringLiteral("Resultado de Floyd-Warshall: costo mínimo entre cada par de nodos."));
            d->addWidget(tablaMatriz(g, r.matrizDistancias, std::vector<int>()));
        }
    }
}
