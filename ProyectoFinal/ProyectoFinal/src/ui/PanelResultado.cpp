#include "PanelResultado.h"
#include "Tema.h"
#include "core/Recorrer.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

using grafos::recorrer;

namespace {

const int MAXIMO_NODOS_EN_MATRIZ = 60; // solo limita lo que se MUESTRA, no el grafo

QString nombreDe(const grafos::Grafo& g, int i) { return QString::fromStdString(g.nodo(i).nombre); }

QString formato(double v) {
    if (!(v < grafos::INFINITO)) return QStringLiteral("∞"); // ∞
    return QString::number(v, 'g', 6);
}

QString escapar(const QString& s) { return s.toHtmlEscaped(); }

QLabel* etiqueta(const QString& texto, const char* nombreObjeto) {
    auto* l = new QLabel(texto);
    l->setObjectName(QString::fromLatin1(nombreObjeto));
    l->setWordWrap(true);
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return l;
}

QTableWidget* tablaCompacta(int filas, int columnas) {
    auto* t = new QTableWidget(filas, columnas);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->setFocusPolicy(Qt::NoFocus);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    t->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    return t;
}

void celda(QTableWidget* t, int fila, int columna, const QString& texto, bool resaltar) {
    auto* item = new QTableWidgetItem(texto);
    item->setTextAlignment(Qt::AlignCenter);
    if (resaltar) {
        item->setBackground(tema::caminoFondo());
        item->setForeground(tema::camino());
    }
    t->setItem(fila, columna, item);
}

// Alto justo para que la tabla no necesite barra vertical.
void ajustarAlto(QTableWidget* t) {
    t->resizeRowsToContents();
    int alto = t->horizontalHeader()->isVisible() ? t->horizontalHeader()->sizeHint().height() : 0;
    recorrer(0, t->rowCount(), [&](int f) { alto += t->rowHeight(f); });
    t->setFixedHeight(alto + 2 * t->frameWidth() + 18); // + espacio para la barra horizontal
}

QTableWidget* tablaMatriz(const grafos::Grafo& g, const std::vector<std::vector<double>>& m,
                          const std::vector<int>& camino) {
    const int n = static_cast<int>(m.size());
    auto* t = tablaCompacta(n, n);
    QStringList encabezados;
    recorrer(0, n, [&](int i) { encabezados << nombreDe(g, i); });
    t->setHorizontalHeaderLabels(encabezados);
    t->setVerticalHeaderLabels(encabezados);

    // Celdas que corresponden a pasos del camino (para resaltarlas)
    std::vector<std::vector<bool>> enCamino(n, std::vector<bool>(n, false));
    recorrer(0, static_cast<int>(camino.size()) - 1, [&](int i) { enCamino[camino[i]][camino[i + 1]] = true; });

    recorrer(0, n, [&](int i) {
        recorrer(0, n, [&](int j) { celda(t, i, j, formato(m[i][j]), enCamino[i][j]); });
    });
    ajustarAlto(t);
    return t;
}

} // namespace

PanelResultado::PanelResultado(QWidget* parent) : QFrame(parent) {
    setObjectName(QStringLiteral("tarjeta"));
    disposicion_ = new QVBoxLayout(this);
    disposicion_->setContentsMargins(14, 14, 14, 14);
    mostrarVacio();
}

QVBoxLayout* PanelResultado::nuevoContenido() {
    delete contenido_; // borra el contenido anterior con todos sus hijos
    contenido_ = new QWidget(this);
    auto* capa = new QVBoxLayout(contenido_);
    capa->setContentsMargins(0, 0, 0, 0);
    capa->setSpacing(6);
    disposicion_->addWidget(contenido_);
    return capa;
}

void PanelResultado::mostrarVacio() {
    auto* capa = nuevoContenido();
    capa->addWidget(etiqueta(QStringLiteral("Agregá nodos y aristas, o cargá el ejemplo, y calculá el camino."), "pista"));
}

void PanelResultado::mostrarError(const QString& mensaje) {
    auto* capa = nuevoContenido();
    capa->addWidget(etiqueta(mensaje, "error"));
}

void PanelResultado::mostrar(const grafos::Grafo& g, const grafos::Resultado& r) {
    if (!r.ok) {
        mostrarError(QString::fromStdString(r.error));
        return;
    }
    auto* capa = nuevoContenido();
    const QString algoritmo = QString::fromStdString(grafos::nombreAlgoritmo(r.algoritmo));

    // Costo total
    capa->addWidget(etiqueta(formato(r.costo), "costo"));
    capa->addWidget(etiqueta(QStringLiteral("costo total · ") + algoritmo, "pista"));

    // Camino como "chips"
    QStringList chips;
    recorrer(0, static_cast<int>(r.camino.size()), [&](int i) {
        chips << QStringLiteral("<span style=\"background-color:#d9f0ee;\">&nbsp;%1&nbsp;</span>")
                     .arg(escapar(nombreDe(g, r.camino[i])));
    });
    auto* camino = etiqueta(chips.join(QStringLiteral(" &rarr; ")), "camino");
    camino->setTextFormat(Qt::RichText);
    capa->addWidget(camino);
    capa->addWidget(etiqueta(QStringLiteral("%1 pasos").arg(static_cast<int>(r.camino.size()) - 1), "pista"));

    if (!r.ordenObligatorios.empty()) {
        QStringList orden;
        recorrer(0, static_cast<int>(r.ordenObligatorios.size()),
                 [&](int i) { orden << nombreDe(g, r.ordenObligatorios[i]); });
        capa->addWidget(etiqueta(QStringLiteral("Orden de paso por los obligatorios: ") + orden.join(", "), "pista"));
    }

    // Costo mínimo desde el origen a cada nodo
    const int n = g.cantidadNodos();
    capa->addSpacing(8);
    capa->addWidget(etiqueta(QStringLiteral("Costo mínimo desde ") + nombreDe(g, r.origen), "tituloTarjeta"));
    auto* distancias = tablaCompacta(1, n);
    QStringList encabezados;
    recorrer(0, n, [&](int i) {
        encabezados << nombreDe(g, i);
        const bool fuera = i < static_cast<int>(r.excluido.size()) && r.excluido[i];
        celda(distancias, 0, i, fuera ? QStringLiteral("—") : formato(r.distDesdeOrigen[i]), i == r.destino);
    });
    distancias->setHorizontalHeaderLabels(encabezados);
    distancias->verticalHeader()->hide();
    ajustarAlto(distancias);
    capa->addWidget(distancias);
    capa->addWidget(etiqueta(QStringLiteral("— = nodo bloqueado o filtrado · ∞ = inalcanzable"), "pista"));

    // Matrices (solo Floyd-Warshall)
    if (r.algoritmo == grafos::Algoritmo::Floyd) {
        capa->addSpacing(8);
        if (n > MAXIMO_NODOS_EN_MATRIZ) {
            capa->addWidget(etiqueta(QStringLiteral("Las matrices tienen %1 × %1 celdas: son demasiado grandes para mostrarlas, "
                                                    "pero el cálculo se hizo igual.").arg(n), "pista"));
        } else {
            capa->addWidget(etiqueta(QStringLiteral("Matriz de adyacencia (incluye el peso del nodo al que se entra)"), "tituloTarjeta"));
            capa->addWidget(tablaMatriz(g, r.matrizAdyacencia, r.camino));
            capa->addSpacing(8);
            capa->addWidget(etiqueta(QStringLiteral("Matriz de distancias mínimas (después de Floyd-Warshall)"), "tituloTarjeta"));
            capa->addWidget(tablaMatriz(g, r.matrizDistancias, std::vector<int>()));
        }
    }
    capa->addStretch(1);
}
