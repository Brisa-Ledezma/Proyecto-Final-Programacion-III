#include "PanelResultado.h"
#include "Tema.h"
#include "core/Recorrer.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFontMetrics>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

using grafos::recorrer;

namespace {

using grafos::MAXIMO_NODOS_EN_MATRIZ; // solo limita lo que se MUESTRA, no el grafo
const int CHIPS_POR_FILA = 7;
const int ALTO_ENCABEZADO = 40; // px base de la fila de títulos de las tablas de resultado

QString nombreDe(const grafos::Grafo& g, int i) { return QString::fromStdString(g.nodo(i).nombre); }

QString formato(double v) {
    if (!(v < grafos::INFINITO)) return QStringLiteral("INF");
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
    t->horizontalHeader()->setMinimumSectionSize(tema::px(54));
    t->horizontalHeader()->setFixedHeight(tema::px(ALTO_ENCABEZADO)); // alto conocido: no depende de cuándo se aplica el estilo
    t->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    t->verticalHeader()->setDefaultSectionSize(tema::px(36));
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
    // isHidden() y no isVisible(): la tabla todavía no se mostró y isVisible() daría false (alto 0 de encabezado)
    int alto = t->horizontalHeader()->isHidden() ? 0 : t->horizontalHeader()->maximumHeight();
    alto += t->verticalHeader()->defaultSectionSize() * t->rowCount();
    t->setFixedHeight(alto + 2 * t->frameWidth() + tema::px(20)); // + lugar para la barra horizontal si hiciera falta
}

QTableWidget* tablaMatriz(const grafos::Grafo& g, const std::vector<std::vector<double>>& m,
                          const std::vector<int>& camino) {
    const int n = static_cast<int>(m.size());
    auto* t = tablaCompacta(n, n);

    // Ancho fijo para todas las columnas, calculado una sola vez con el texto más largo, y ANTES de
    // cargar encabezados y celdas. "Ajustar al contenido" mide todas las filas de cada columna
    // (n² mediciones con estilo) cada vez que cambia algo: con 40-60 nodos la ventana quedaba trabada.
    const QFontMetrics medida(t->font());
    int ancho = tema::px(54);
    recorrer(0, n, [&](int i) {
        recorrer(0, n, [&](int j) {
            const int w = medida.horizontalAdvance(formato(m[i][j])) + tema::px(30);
            if (w > ancho) ancho = w;
        });
    });
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    t->horizontalHeader()->setDefaultSectionSize(ancho);

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
    // Títulos alineados igual que sus celdas: TRAMO y NOTA a la izquierda, el resto centrado
    recorrer(0, 5, [&](int c) {
        const bool izquierda = (c == 1 || c == 4);
        t->horizontalHeaderItem(c)->setTextAlignment(izquierda ? (Qt::AlignVCenter | Qt::AlignLeft)
                                                               : Qt::Alignment(Qt::AlignCenter));
    });
    // Las cinco columnas miden lo mismo: se reparten el ancho de la tabla por igual
    recorrer(0, 5, [&](int c) { t->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch); });

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
    QLabel* rotuloCosto = etiqueta(QStringLiteral("costo total"), "rotulo");
    QLabel* detalleCosto = etiqueta(algoritmo + QStringLiteral(" · ") + QString::number(pasos) +
                                        (pasos == 1 ? QStringLiteral(" paso") : QStringLiteral(" pasos")), "pista");
    rotuloCosto->setWordWrap(false); // en una sola línea: con nombres largos se partía en tres
    detalleCosto->setWordWrap(false);
    datos->addWidget(rotuloCosto);
    datos->addWidget(detalleCosto);
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

    if (!r.aviso.empty()) resumen->addWidget(etiqueta(QString::fromStdString(r.aviso), "pista"));

    // ---------- Detalle del recorrido ----------
    if (pasos > 0) {
        QVBoxLayout* detalle = agregarTarjeta(capa, QStringLiteral("Detalle del recorrido"),
            QStringLiteral("Costo de cada paso = peso de la arista en el sentido en que se recorre."));
        detalle->addWidget(tablaDetalle(g, r));
    } else {
        QVBoxLayout* detalle = agregarTarjeta(capa, QStringLiteral("Detalle del recorrido"), QString());
        detalle->addWidget(etiqueta(QStringLiteral("El origen y el destino son el mismo nodo: no hay pasos."), "pista"));
    }

    const int n = g.cantidadNodos();

    // ---------- Matrices ----------
    if (n > MAXIMO_NODOS_EN_MATRIZ) {
        QVBoxLayout* m = agregarTarjeta(capa, QStringLiteral("Matriz de adyacencia"), QString());
        m->addWidget(etiqueta(QStringLiteral("La matriz tiene %1 × %1 celdas: es demasiado grande para "
                                             "mostrarla, pero el cálculo se hizo igual.").arg(n), "pista"));
        return;
    }
    // Se muestra con cualquier algoritmo
    QVBoxLayout* a = agregarTarjeta(capa, QStringLiteral("Matriz de adyacencia"),
        QStringLiteral("Peso del arco directo fila → columna (0 en la diagonal, INF si no hay arco o está cortado). "
                       "En rosa, los arcos que usa el camino."));
    a->addWidget(tablaMatriz(g, r.matrizAdyacencia, r.camino));
}
