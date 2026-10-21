#include "VentanaPrincipal.h"
#include "ItemsGrafo.h"
#include "LienzoGrafo.h"
#include "PanelResultado.h"
#include "core/Recorrer.h"

#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

using grafos::recorrer;

namespace {

QString aQString(const std::string& s) { return QString::fromStdString(s); }

QLabel* pista(const QString& texto) {
    auto* l = new QLabel(texto);
    l->setObjectName(QStringLiteral("pista"));
    l->setWordWrap(true);
    return l;
}

QDoubleSpinBox* crearSpin(double valor) {
    auto* s = new QDoubleSpinBox;
    s->setRange(-1000000.0, 1000000.0);
    s->setDecimals(0); // solo enteros
    s->setValue(valor);
    s->setMinimumWidth(70);
    return s;
}

QDoubleSpinBox* spinDeTabla(double valor) {
    auto* s = crearSpin(valor);
    s->setButtonSymbols(QAbstractSpinBox::NoButtons);
    s->setMinimumWidth(60);
    return s;
}

QWidget* centrado(QWidget* w) {
    auto* c = new QWidget;
    auto* l = new QHBoxLayout(c);
    l->setContentsMargins(0, 0, 0, 0);
    l->setAlignment(Qt::AlignCenter);
    l->addWidget(w);
    return c;
}

QTableWidget* crearTabla(const QStringList& columnas, int columnaElastica) {
    auto* t = new QTableWidget(0, static_cast<int>(columnas.size()));
    t->setHorizontalHeaderLabels(columnas);
    t->verticalHeader()->hide();
    t->setShowGrid(false);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->setFocusPolicy(Qt::NoFocus);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setSectionResizeMode(columnaElastica, QHeaderView::Stretch);
    t->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    return t;
}

// La tabla crece con sus filas hasta un máximo; después aparece la barra.
void ajustarAltoTabla(QTableWidget* t) {
    const int fila = 34;
    const int alto = t->horizontalHeader()->sizeHint().height() + fila * t->rowCount() + 6;
    t->setFixedHeight(alto < 260 ? alto : 260);
    recorrer(0, t->rowCount(), [&](int f) { t->setRowHeight(f, fila); });
}

QTableWidgetItem* itemTexto(const QString& texto) {
    auto* i = new QTableWidgetItem(texto);
    i->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    return i;
}

// Rellena un combo con los nombres de los nodos, conservando la selección si se puede.
void llenarCombo(QComboBox* combo, const grafos::Grafo& g, int indicePorDefecto) {
    const QSignalBlocker bloqueo(combo);
    const QString anterior = combo->currentText();
    combo->clear();
    recorrer(0, g.cantidadNodos(), [&](int i) { combo->addItem(aQString(g.nodo(i).nombre)); });
    const int encontrado = combo->findText(anterior);
    if (encontrado >= 0) combo->setCurrentIndex(encontrado);
    else if (indicePorDefecto >= 0 && indicePorDefecto < combo->count()) combo->setCurrentIndex(indicePorDefecto);
}

} // namespace

// ============================================================
//  Construcción
// ============================================================
VentanaPrincipal::VentanaPrincipal(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Caminos en grafos — Programación III"));
    construirInterfaz();
    refrescarTodo();
}

QFrame* VentanaPrincipal::tarjeta(const QString& titulo, QVBoxLayout*& contenido) {
    auto* marco = new QFrame;
    marco->setObjectName(QStringLiteral("tarjeta"));
    contenido = new QVBoxLayout(marco);
    contenido->setContentsMargins(14, 12, 14, 14);
    contenido->setSpacing(8);
    auto* t = new QLabel(titulo);
    t->setObjectName(QStringLiteral("tituloTarjeta"));
    contenido->addWidget(t);
    return marco;
}

void VentanaPrincipal::construirInterfaz() {
    auto* fondo = new QWidget;
    fondo->setObjectName(QStringLiteral("fondo"));
    auto* principal = new QVBoxLayout(fondo);
    principal->setContentsMargins(20, 16, 20, 16);
    principal->setSpacing(10);

    // Encabezado
    auto* titulo = new QLabel(QStringLiteral("Caminos en grafos"));
    titulo->setObjectName(QStringLiteral("titulo"));
    auto* subtitulo = new QLabel(QStringLiteral("Cargá nodos, aristas y restricciones, y elegí con qué algoritmo resolver."));
    subtitulo->setObjectName(QStringLiteral("subtitulo"));
    principal->addWidget(titulo);
    principal->addWidget(subtitulo);

    auto* columnas = new QHBoxLayout;
    columnas->setSpacing(16);
    principal->addLayout(columnas, 1);

    // ---- Columna izquierda (tarjetas con scroll) ----
    auto* contenidoIzquierdo = new QWidget;
    contenidoIzquierdo->setObjectName(QStringLiteral("contenidoIzquierdo"));
    auto* izquierda = new QVBoxLayout(contenidoIzquierdo);
    izquierda->setContentsMargins(0, 0, 6, 0);
    izquierda->setSpacing(14);
    izquierda->addWidget(tarjetaNodos());
    izquierda->addWidget(tarjetaAristas());
    izquierda->addWidget(tarjetaFiltros());
    izquierda->addWidget(tarjetaResolver());
    izquierda->addStretch(1);

    auto* scrollIzquierdo = new QScrollArea;
    scrollIzquierdo->setObjectName(QStringLiteral("scrollIzquierdo"));
    scrollIzquierdo->setWidget(contenidoIzquierdo);
    scrollIzquierdo->setWidgetResizable(true);
    scrollIzquierdo->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollIzquierdo->setFixedWidth(450);
    columnas->addWidget(scrollIzquierdo);

    // ---- Columna derecha: barra + lienzo + resultado ----
    auto* derecha = new QVBoxLayout;
    derecha->setSpacing(8);

    auto* barra = new QHBoxLayout;
    barra->addWidget(pista(QStringLiteral("Clic en el fondo: nuevo nodo · Arrastrar un nodo: moverlo · "
                                          "Clic derecho: opciones · Ctrl + rueda: zoom")), 1);
    auto* botonRepetir = new QPushButton(QStringLiteral("Ver recorrido de nuevo"));
    auto* botonAjustar = new QPushButton(QStringLiteral("Ajustar vista"));
    barra->addWidget(botonRepetir);
    barra->addWidget(botonAjustar);
    derecha->addLayout(barra);

    lienzo_ = new LienzoGrafo(&grafo_);
    lienzo_->alHacerClicEnVacio = [this](QPointF p) { agregarNodo(p, true); };
    lienzo_->alPedirMenuNodo = [this](int i, QPoint p) { menuNodo(i, p); };
    lienzo_->alPedirMenuArista = [this](int i, QPoint p) { menuArista(i, p); };

    resultado_ = new PanelResultado;
    auto* scrollResultado = new QScrollArea;
    scrollResultado->setWidget(resultado_);
    scrollResultado->setWidgetResizable(true);

    auto* divisor = new QSplitter(Qt::Vertical);
    divisor->addWidget(lienzo_);
    divisor->addWidget(scrollResultado);
    divisor->setStretchFactor(0, 3);
    divisor->setStretchFactor(1, 2);
    divisor->setChildrenCollapsible(false);
    derecha->addWidget(divisor, 1);
    columnas->addLayout(derecha, 1);

    connect(botonRepetir, &QPushButton::clicked, this, [this]() { lienzo_->repetirAnimacion(); });
    connect(botonAjustar, &QPushButton::clicked, this, [this]() { lienzo_->ajustarVista(); });

    setCentralWidget(fondo);
}

QFrame* VentanaPrincipal::tarjetaNodos() {
    QVBoxLayout* c = nullptr;
    QFrame* marco = tarjeta(QStringLiteral("Nodos"), c);

    auto* fila = new QHBoxLayout;
    nombreNodo_ = new QLineEdit;
    nombreNodo_->setPlaceholderText(QStringLiteral("Nombre (vacío = automático)"));
    // Solo letras: los números se confundirían con el peso o el número del nodo
    nombreNodo_->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("\\p{L}{0,%1}").arg(grafos::Grafo::LARGO_MAXIMO_NOMBRE)), nombreNodo_));
    pesoNodo_ = crearSpin(0.0);
    pesoNodo_->setToolTip(QStringLiteral("Peso del nodo"));
    auto* agregar = new QPushButton(QStringLiteral("Agregar"));
    fila->addWidget(nombreNodo_, 1);
    fila->addWidget(new QLabel(QStringLiteral("Peso")));
    fila->addWidget(pesoNodo_);
    fila->addWidget(agregar);
    c->addLayout(fila);

    tablaNodos_ = crearTabla({QStringLiteral("#"), QStringLiteral("Nodo"), QStringLiteral("Peso"),
                              QStringLiteral("Bloq."), QStringLiteral("Oblig."), QString()}, 1);
    c->addWidget(tablaNodos_);
    c->addWidget(pista(QStringLiteral("El peso del nodo se suma cada vez que el camino entra en él (el origen no cuenta). "
                                      "El nombre solo admite letras. El # es el número que usan los filtros par/impar.")));

    connect(agregar, &QPushButton::clicked, this, [this]() { agregarNodo(QPointF(), false); });
    connect(nombreNodo_, &QLineEdit::returnPressed, this, [this]() { agregarNodo(QPointF(), false); });
    return marco;
}

QFrame* VentanaPrincipal::tarjetaAristas() {
    QVBoxLayout* c = nullptr;
    QFrame* marco = tarjeta(QStringLiteral("Aristas"), c);

    auto* fila1 = new QHBoxLayout;
    desde_ = new QComboBox;
    hasta_ = new QComboBox;
    tipoArista_ = new QComboBox;
    tipoArista_->addItem(QStringLiteral("No dirigida"));
    tipoArista_->addItem(QStringLiteral("Dirigida"));
    fila1->addWidget(new QLabel(QStringLiteral("De")));
    fila1->addWidget(desde_, 1);
    fila1->addWidget(new QLabel(QStringLiteral("a")));
    fila1->addWidget(hasta_, 1);
    fila1->addWidget(tipoArista_, 1);
    c->addLayout(fila1);

    auto* fila2 = new QHBoxLayout;
    etiquetaIda_ = new QLabel(QStringLiteral("Ida"));
    etiquetaVuelta_ = new QLabel(QStringLiteral("Vuelta"));
    pesoIda_ = crearSpin(1.0);
    pesoVuelta_ = crearSpin(1.0);
    auto* agregar = new QPushButton(QStringLiteral("Agregar"));
    fila2->addWidget(etiquetaIda_);
    fila2->addWidget(pesoIda_);
    fila2->addWidget(etiquetaVuelta_);
    fila2->addWidget(pesoVuelta_);
    fila2->addStretch(1);
    fila2->addWidget(agregar);
    c->addLayout(fila2);

    tablaAristas_ = crearTabla({QStringLiteral("Arista"), QStringLiteral("Ida"), QStringLiteral("Vuelta"),
                                QStringLiteral("Tipo"), QStringLiteral("Bloq."), QString()}, 0);
    c->addWidget(tablaAristas_);
    c->addWidget(pista(QStringLiteral("Dirigida: un solo peso. No dirigida: peso de ida (De → a) y de vuelta (a → De). "
                                      "En el dibujo se ve como «ida ⇄ vuelta» cuando son distintos.")));

    // Si es dirigida, el peso de vuelta no aplica
    connect(tipoArista_, &QComboBox::currentIndexChanged, this, [this](int indice) {
        const bool dirigida = indice == 1;
        pesoVuelta_->setEnabled(!dirigida);
        etiquetaVuelta_->setEnabled(!dirigida);
        etiquetaIda_->setText(dirigida ? QStringLiteral("Peso") : QStringLiteral("Ida"));
    });
    connect(agregar, &QPushButton::clicked, this, [this]() { agregarArista(); });
    return marco;
}

QFrame* VentanaPrincipal::tarjetaFiltros() {
    QVBoxLayout* c = nullptr;
    QFrame* marco = tarjeta(QStringLiteral("Filtros"), c);
    evitarPares_ = new QCheckBox(QStringLiteral("Evitar nodos pares (#2, #4, #6…)"));
    evitarImpares_ = new QCheckBox(QStringLiteral("Evitar nodos impares (#1, #3, #5…)"));
    c->addWidget(evitarPares_);
    c->addWidget(evitarImpares_);
    c->addWidget(pista(QStringLiteral("Para evitar un nodo o una arista puntual, tildá «Bloq.» en su tabla "
                                      "o hacé clic derecho sobre él en el dibujo.")));

    const auto alCambiar = [this](bool) {
        lienzo_->setFiltros(filtros());
        cambio();
    };
    connect(evitarPares_, &QCheckBox::toggled, this, alCambiar);
    connect(evitarImpares_, &QCheckBox::toggled, this, alCambiar);
    return marco;
}

QFrame* VentanaPrincipal::tarjetaResolver() {
    QVBoxLayout* c = nullptr;
    QFrame* marco = tarjeta(QStringLiteral("Resolver"), c);

    auto* fila1 = new QHBoxLayout;
    origen_ = new QComboBox;
    destino_ = new QComboBox;
    fila1->addWidget(new QLabel(QStringLiteral("Origen")));
    fila1->addWidget(origen_, 1);
    fila1->addWidget(new QLabel(QStringLiteral("Destino")));
    fila1->addWidget(destino_, 1);
    c->addLayout(fila1);

    auto* fila2 = new QHBoxLayout;
    algoritmo_ = new QComboBox;
    algoritmo_->addItem(QStringLiteral("Dijkstra"));
    algoritmo_->addItem(QStringLiteral("Bellman-Ford"));
    algoritmo_->addItem(QStringLiteral("Matriz de adyacencia (Floyd-Warshall)"));
    fila2->addWidget(new QLabel(QStringLiteral("Algoritmo")));
    fila2->addWidget(algoritmo_, 1);
    c->addLayout(fila2);

    animar_ = new QCheckBox(QStringLiteral("Mostrar el recorrido paso a paso"));
    animar_->setChecked(true);
    c->addWidget(animar_);

    auto* fila3 = new QHBoxLayout;
    auto* calcularBoton = new QPushButton(QStringLiteral("Calcular camino"));
    calcularBoton->setObjectName(QStringLiteral("principal"));
    auto* ejemplo = new QPushButton(QStringLiteral("Cargar ejemplo"));
    auto* limpiar = new QPushButton(QStringLiteral("Limpiar todo"));
    fila3->addWidget(calcularBoton);
    fila3->addWidget(ejemplo);
    fila3->addWidget(limpiar);
    c->addLayout(fila3);
    c->addWidget(pista(QStringLiteral("Los nodos obligatorios se visitan en el orden que dé menor costo (hasta %1).")
                           .arg(grafos::MAXIMO_OBLIGATORIOS)));

    const auto alCambiarExtremos = [this](int) {
        lienzo_->setOrigenDestino(origen_->currentIndex(), destino_->currentIndex());
        cambio();
    };
    connect(origen_, &QComboBox::currentIndexChanged, this, alCambiarExtremos);
    connect(destino_, &QComboBox::currentIndexChanged, this, alCambiarExtremos);
    connect(algoritmo_, &QComboBox::currentIndexChanged, this, [this](int) { cambio(); });
    connect(calcularBoton, &QPushButton::clicked, this, [this]() { calcular(true); });
    connect(ejemplo, &QPushButton::clicked, this, [this]() { cargarEjemplo(); });
    connect(limpiar, &QPushButton::clicked, this, [this]() { limpiarTodo(); });
    return marco;
}

// ============================================================
//  Refresco
// ============================================================
void VentanaPrincipal::refrescarTodo() {
    refrescarCombos();
    refrescarTablaNodos();
    refrescarTablaAristas();
    lienzo_->setFiltros(filtros());
    lienzo_->setOrigenDestino(origen_->currentIndex(), destino_->currentIndex());
    lienzo_->reconstruir();
    cambio();
}

void VentanaPrincipal::refrescarCombos() {
    const int n = grafo_.cantidadNodos();
    llenarCombo(desde_, grafo_, 0);
    llenarCombo(hasta_, grafo_, n > 1 ? 1 : 0);
    llenarCombo(origen_, grafo_, 0);
    llenarCombo(destino_, grafo_, n - 1);
}

void VentanaPrincipal::refrescarTablaNodos() {
    const int n = grafo_.cantidadNodos();
    tablaNodos_->setRowCount(0); // borra también los widgets de las celdas
    tablaNodos_->setRowCount(n);
    recorrer(0, n, [&](int i) {
        const grafos::Nodo& nodo = grafo_.nodo(i);
        tablaNodos_->setItem(i, 0, itemTexto(QStringLiteral("#%1").arg(grafos::Grafo::numeroDeNodo(i))));
        tablaNodos_->setItem(i, 1, itemTexto(aQString(nodo.nombre)));

        auto* peso = spinDeTabla(nodo.peso);
        connect(peso, &QDoubleSpinBox::valueChanged, this, [this, i](double v) {
            grafo_.nodo(i).peso = static_cast<int>(v);
            lienzo_->reconstruir();
            cambio();
        });
        tablaNodos_->setCellWidget(i, 2, peso);

        auto* bloqueado = new QCheckBox;
        bloqueado->setChecked(nodo.bloqueado);
        connect(bloqueado, &QCheckBox::toggled, this, [this, i](bool v) {
            grafo_.nodo(i).bloqueado = v;
            lienzo_->setFiltros(filtros());
            cambio();
        });
        tablaNodos_->setCellWidget(i, 3, centrado(bloqueado));

        auto* obligatorio = new QCheckBox;
        obligatorio->setChecked(nodo.obligatorio);
        connect(obligatorio, &QCheckBox::toggled, this, [this, i](bool v) {
            grafo_.nodo(i).obligatorio = v;
            lienzo_->setFiltros(filtros());
            cambio();
        });
        tablaNodos_->setCellWidget(i, 4, centrado(obligatorio));

        auto* quitar = new QToolButton;
        quitar->setObjectName(QStringLiteral("quitar"));
        quitar->setText(QStringLiteral("✕"));
        quitar->setToolTip(QStringLiteral("Quitar nodo"));
        // Diferido: no se puede borrar la tabla mientras se procesa el clic de su botón
        connect(quitar, &QToolButton::clicked, this, [this, i]() {
            QTimer::singleShot(0, this, [this, i]() { eliminarNodo(i); });
        });
        tablaNodos_->setCellWidget(i, 5, quitar);
    });
    ajustarAltoTabla(tablaNodos_);
}

void VentanaPrincipal::refrescarTablaAristas() {
    const int m = grafo_.cantidadAristas();
    tablaAristas_->setRowCount(0);
    tablaAristas_->setRowCount(m);
    recorrer(0, m, [&](int k) {
        const grafos::Arista& e = grafo_.arista(k);
        tablaAristas_->setItem(k, 0, itemTexto(aQString(grafo_.describirArista(k))));

        auto* ida = spinDeTabla(e.pesoIda);
        connect(ida, &QDoubleSpinBox::valueChanged, this, [this, k](double v) {
            grafos::Arista& a = grafo_.arista(k);
            a.pesoIda = static_cast<int>(v);
            if (a.dirigida) a.pesoVuelta = a.pesoIda;
            lienzo_->reconstruir();
            cambio();
        });
        tablaAristas_->setCellWidget(k, 1, ida);

        if (e.dirigida) {
            tablaAristas_->setItem(k, 2, itemTexto(QStringLiteral("—")));
        } else {
            auto* vuelta = spinDeTabla(e.pesoVuelta);
            connect(vuelta, &QDoubleSpinBox::valueChanged, this, [this, k](double v) {
                grafo_.arista(k).pesoVuelta = static_cast<int>(v);
                lienzo_->reconstruir();
                cambio();
            });
            tablaAristas_->setCellWidget(k, 2, vuelta);
        }

        tablaAristas_->setItem(k, 3, itemTexto(e.dirigida ? QStringLiteral("Dirigida") : QStringLiteral("No dirigida")));

        auto* bloqueada = new QCheckBox;
        bloqueada->setChecked(e.bloqueada);
        connect(bloqueada, &QCheckBox::toggled, this, [this, k](bool v) {
            grafo_.arista(k).bloqueada = v;
            lienzo_->setFiltros(filtros());
            cambio();
        });
        tablaAristas_->setCellWidget(k, 4, centrado(bloqueada));

        auto* quitar = new QToolButton;
        quitar->setObjectName(QStringLiteral("quitar"));
        quitar->setText(QStringLiteral("✕"));
        quitar->setToolTip(QStringLiteral("Quitar arista"));
        connect(quitar, &QToolButton::clicked, this, [this, k]() {
            QTimer::singleShot(0, this, [this, k]() { eliminarArista(k); });
        });
        tablaAristas_->setCellWidget(k, 5, quitar);
    });
    ajustarAltoTabla(tablaAristas_);
}

// Después del primer cálculo, cualquier cambio vuelve a calcular (como el prototipo).
void VentanaPrincipal::cambio() {
    if (calculado_) {
        calcular(false);
        return;
    }
    lienzo_->setResultado(nullptr, false);
}

// ============================================================
//  Acciones
// ============================================================
void VentanaPrincipal::agregarNodo(QPointF posicion, bool posicionElegida) {
    const QString escrito = nombreNodo_->text().trimmed();
    const std::string nombre = escrito.isEmpty() ? grafo_.siguienteNombreLibre() : escrito.toStdString();
    const std::string error = grafo_.validarNombre(nombre);
    if (!error.empty()) {
        QMessageBox::warning(this, QStringLiteral("No se puede agregar el nodo"), aQString(error));
        return;
    }
    const double radio = radioParaNombre(aQString(nombre));
    const QPointF deseada = posicionElegida ? posicion : lienzo_->centroVisible();
    const QPointF libre = lienzo_->posicionLibre(deseada, radio, -1); // nunca encima de otro nodo
    grafo_.agregarNodo(nombre, static_cast<int>(pesoNodo_->value()), libre.x(), libre.y());
    nombreNodo_->clear();
    refrescarTodo();
}

void VentanaPrincipal::agregarArista() {
    const int a = desde_->currentIndex();
    const int b = hasta_->currentIndex();
    const bool dirigida = tipoArista_->currentIndex() == 1;
    const std::string error = grafo_.validarArista(a, b, dirigida);
    if (!error.empty()) {
        QMessageBox::warning(this, QStringLiteral("No se puede agregar la arista"), aQString(error));
        return;
    }
    grafo_.agregarArista(a, b, static_cast<int>(pesoIda_->value()), static_cast<int>(pesoVuelta_->value()), dirigida);
    refrescarTodo();
}

void VentanaPrincipal::eliminarNodo(int indice) {
    grafo_.eliminarNodo(indice);
    refrescarTodo();
}

void VentanaPrincipal::eliminarArista(int indice) {
    grafo_.eliminarArista(indice);
    refrescarTodo();
}

grafos::Filtros VentanaPrincipal::filtros() const {
    grafos::Filtros f;
    f.evitarPares = evitarPares_->isChecked();
    f.evitarImpares = evitarImpares_->isChecked();
    return f;
}

grafos::Algoritmo VentanaPrincipal::algoritmoElegido() const {
    if (algoritmo_->currentIndex() == 1) return grafos::Algoritmo::BellmanFord;
    if (algoritmo_->currentIndex() == 2) return grafos::Algoritmo::Floyd;
    return grafos::Algoritmo::Dijkstra;
}

void VentanaPrincipal::calcular(bool animar) {
    calculado_ = true;
    const grafos::Resultado r = grafos::resolver(grafo_, origen_->currentIndex(), destino_->currentIndex(),
                                                 algoritmoElegido(), filtros());
    resultado_->mostrar(grafo_, r);
    lienzo_->setResultado(&r, animar && animar_->isChecked());
}

void VentanaPrincipal::cargarEjemplo() {
    int origen = 0;
    int destino = 0;
    grafo_.cargarEjemplo(origen, destino);
    {
        const QSignalBlocker b1(evitarPares_);
        const QSignalBlocker b2(evitarImpares_);
        evitarPares_->setChecked(false);
        evitarImpares_->setChecked(false);
    }
    refrescarCombos();
    {
        const QSignalBlocker b1(origen_);
        const QSignalBlocker b2(destino_);
        origen_->setCurrentIndex(origen);
        destino_->setCurrentIndex(destino);
    }
    refrescarTablaNodos();
    refrescarTablaAristas();
    lienzo_->setFiltros(filtros());
    lienzo_->setOrigenDestino(origen, destino);
    lienzo_->reconstruir();
    lienzo_->ajustarVista();
    calcular(true);
}

void VentanaPrincipal::limpiarTodo() {
    if (grafo_.cantidadNodos() > 0) {
        const auto respuesta = QMessageBox::question(this, QStringLiteral("Limpiar todo"),
                                                     QStringLiteral("¿Borrar todos los nodos, aristas y el resultado?"));
        if (respuesta != QMessageBox::Yes) return;
    }
    grafo_.limpiar();
    calculado_ = false;
    {
        const QSignalBlocker b1(evitarPares_);
        const QSignalBlocker b2(evitarImpares_);
        evitarPares_->setChecked(false);
        evitarImpares_->setChecked(false);
    }
    resultado_->mostrarVacio();
    refrescarTodo();
    lienzo_->ajustarVista();
}

// ============================================================
//  Menús contextuales del dibujo
// ============================================================
void VentanaPrincipal::menuNodo(int indice, QPoint posicionPantalla) {
    if (indice < 0 || indice >= grafo_.cantidadNodos()) return;
    const grafos::Nodo& nodo = grafo_.nodo(indice);

    QMenu menu(this);
    QAction* bloquear = menu.addAction(nodo.bloqueado ? QStringLiteral("Desbloquear nodo")
                                                      : QStringLiteral("Bloquear nodo (evitarlo)"));
    QAction* obligatorio = menu.addAction(nodo.obligatorio ? QStringLiteral("Quitar de obligatorios")
                                                           : QStringLiteral("Marcar como obligatorio"));
    menu.addSeparator();
    QAction* comoOrigen = menu.addAction(QStringLiteral("Usar como origen"));
    QAction* comoDestino = menu.addAction(QStringLiteral("Usar como destino"));
    menu.addSeparator();
    QAction* eliminar = menu.addAction(QStringLiteral("Eliminar nodo"));

    QAction* elegida = menu.exec(posicionPantalla);
    if (elegida == nullptr) return;
    if (elegida == comoOrigen) { origen_->setCurrentIndex(indice); return; }
    if (elegida == comoDestino) { destino_->setCurrentIndex(indice); return; }
    if (elegida == bloquear) grafo_.nodo(indice).bloqueado = !nodo.bloqueado;
    if (elegida == obligatorio) grafo_.nodo(indice).obligatorio = !nodo.obligatorio;
    const bool borrar = elegida == eliminar;
    // Diferido: el dibujo se reconstruye cuando termina el evento del clic derecho
    QTimer::singleShot(0, this, [this, indice, borrar]() {
        if (borrar) eliminarNodo(indice);
        else refrescarTodo();
    });
}

void VentanaPrincipal::menuArista(int indice, QPoint posicionPantalla) {
    if (indice < 0 || indice >= grafo_.cantidadAristas()) return;
    const grafos::Arista& arista = grafo_.arista(indice);

    QMenu menu(this);
    QAction* bloquear = menu.addAction(arista.bloqueada ? QStringLiteral("Desbloquear arista")
                                                        : QStringLiteral("Bloquear arista (evitar este camino)"));
    QAction* eliminar = menu.addAction(QStringLiteral("Eliminar arista"));

    QAction* elegida = menu.exec(posicionPantalla);
    if (elegida == nullptr) return;
    if (elegida == bloquear) grafo_.arista(indice).bloqueada = !arista.bloqueada;
    const bool borrar = elegida == eliminar;
    QTimer::singleShot(0, this, [this, indice, borrar]() {
        if (borrar) eliminarArista(indice);
        else refrescarTodo();
    });
}
