#include "VentanaPrincipal.h"
#include "Controles.h"
#include "ItemsGrafo.h"
#include "LienzoGrafo.h"
#include "PanelResultado.h"
#include "Tema.h"
#include "core/Recorrer.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
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
#include <QShortcut>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>
#include <functional>

using grafos::recorrer;

namespace {

const int ANCHO_COLUMNA_IZQUIERDA = 440; // medidas base: se multiplican por tema::escala() con tema::px()
const int ALTO_LIENZO = 440;
const int ALTO_FILA_TABLA = 44;
const int MAXIMO_DESHACER = 100; // ediciones que se pueden deshacer

QString aQString(const std::string& s) { return QString::fromStdString(s); }

QLabel* conNombre(QLabel* l, const char* nombreObjeto) {
    l->setObjectName(QString::fromLatin1(nombreObjeto));
    return l;
}

QLabel* pista(const QString& texto) {
    auto* l = conNombre(new QLabel(texto), "pista");
    l->setWordWrap(true);
    return l;
}

QLabel* rotulo(const QString& texto) { return conNombre(new QLabel(texto), "rotulo"); }

QSpinBox* crearSpin(int valor) {
    auto* s = new SpinSinRueda;
    s->setValue(valor);
    s->setMinimumWidth(tema::px(80));
    return s;
}

QSpinBox* spinDeTabla(int valor) {
    auto* s = new SpinSinRueda;
    s->setObjectName(QStringLiteral("enTabla"));
    s->setValue(valor);
    s->setFixedWidth(tema::px(76));
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
    t->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    t->verticalHeader()->setDefaultSectionSize(tema::px(ALTO_FILA_TABLA));
    t->setShowGrid(false);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->setFocusPolicy(Qt::NoFocus);
    t->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // la página es la que hace scroll
    t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setSectionResizeMode(columnaElastica, QHeaderView::Stretch);
    t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return t;
}

// Alto exacto para mostrar todas las filas, sin barra interna.
void ajustarAltoTabla(QTableWidget* t) {
    const int fila = tema::px(ALTO_FILA_TABLA);
    t->verticalHeader()->setDefaultSectionSize(fila);
    const int alto = t->horizontalHeader()->sizeHint().height() + fila * t->rowCount() + 4;
    t->setFixedHeight(alto);
    t->setVisible(t->rowCount() > 0);
}

QTableWidgetItem* itemTexto(const QString& texto) {
    auto* i = new QTableWidgetItem(texto);
    i->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    return i;
}

QToolButton* botonQuitar(const QString& ayuda) {
    auto* b = new QToolButton;
    b->setObjectName(QStringLiteral("quitar"));
    b->setText(QStringLiteral("✕"));
    b->setToolTip(ayuda);
    b->setCursor(Qt::PointingHandCursor);
    return b;
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
    crearAtajosDeZoom();
    // Ctrl + Z: si el foco está en un campo de texto, ahí deshace lo tipeado; si no, deshace la edición del grafo
    auto* atajoDeshacer = new QShortcut(QKeySequence::Undo, this);
    connect(atajoDeshacer, &QShortcut::activated, this, [this]() { deshacer(); });
    refrescarTodo();
}

// ============================================================
//  Zoom de la letra: Ctrl + (o Ctrl =) agranda, Ctrl - achica, Ctrl 0 vuelve al tamaño inicial
// ============================================================
void VentanaPrincipal::crearAtajosDeZoom() {
    const auto atajo = [this](const QKeySequence& teclas, std::function<void()> accion) {
        auto* a = new QShortcut(teclas, this);
        a->setContext(Qt::ApplicationShortcut);
        connect(a, &QShortcut::activated, this, accion);
    };
    const auto mas = [this]() { cambiarEscala(tema::escala() + tema::PASO_ESCALA); };
    const auto menos = [this]() { cambiarEscala(tema::escala() - tema::PASO_ESCALA); };
    atajo(QKeySequence(Qt::CTRL | Qt::Key_Plus), mas);
    atajo(QKeySequence(Qt::CTRL | Qt::Key_Equal), mas); // en teclados donde "+" necesita Shift
    atajo(QKeySequence(Qt::CTRL | Qt::Key_Minus), menos);
    atajo(QKeySequence(Qt::CTRL | Qt::Key_0), [this]() { cambiarEscala(tema::ESCALA_INICIAL); });
}

void VentanaPrincipal::cambiarEscala(double nueva) {
    nueva = std::round(nueva * 10.0) / 10.0; // evita acumular errores de decimales
    if (nueva < tema::ESCALA_MINIMA) nueva = tema::ESCALA_MINIMA;
    if (nueva > tema::ESCALA_MAXIMA) nueva = tema::ESCALA_MAXIMA;
    const double previa = tema::escala();
    if (std::abs(nueva - previa) < 1e-6) return;

    qApp->setProperty("escalaUI", nueva);
    qApp->setStyleSheet(tema::hojaDeEstilo(qApp->property("carpetaIconos").toString(), nueva));
    izquierda_->setFixedWidth(tema::px(ANCHO_COLUMNA_IZQUIERDA));
    refrescarTablaNodos();   // vuelven a calcular el alto de filas y el ancho de los campos
    refrescarTablaAristas();
    lienzo_->escalarVista(nueva / previa); // el dibujo (nodos y números) crece con la letra
    if (calculado_) calcular(false);       // las tablas del resultado se rearman con la nueva escala
}

QFrame* VentanaPrincipal::tarjeta(const QString& titulo, QVBoxLayout*& contenido) {
    auto* marco = new QFrame;
    marco->setObjectName(QStringLiteral("tarjeta"));
    contenido = new QVBoxLayout(marco);
    contenido->setContentsMargins(22, 18, 22, 22);
    contenido->setSpacing(14);
    contenido->addWidget(conNombre(new QLabel(titulo), "tituloTarjeta"));
    return marco;
}

void VentanaPrincipal::construirInterfaz() {
    // Toda la página va dentro de un único scroll
    auto* pagina = new QWidget;
    pagina->setObjectName(QStringLiteral("pagina"));
    auto* principal = new QVBoxLayout(pagina);
    principal->setContentsMargins(32, 26, 32, 32);
    principal->setSpacing(22);

    // ---- Encabezado ----
    auto* encabezado = new QVBoxLayout;
    encabezado->setSpacing(4);
    auto* filaTitulo = new QHBoxLayout;
    filaTitulo->setSpacing(12);
    filaTitulo->addWidget(conNombre(new QLabel(QStringLiteral("Caminos en grafos")), "titulo"));
    filaTitulo->addWidget(conNombre(new QLabel(QStringLiteral("Programación III")), "etiquetaMateria"), 0, Qt::AlignVCenter);
    filaTitulo->addStretch(1);
    encabezado->addLayout(filaTitulo);
    encabezado->addWidget(conNombre(new QLabel(QStringLiteral(
        "Cargá nodos, aristas y restricciones, y elegí con qué algoritmo resolver.")), "subtitulo"));
    principal->addLayout(encabezado);

    // ---- Dos columnas ----
    auto* columnas = new QHBoxLayout;
    columnas->setSpacing(22);

    auto* izquierda = new QWidget;
    izquierda_ = izquierda;
    izquierda->setFixedWidth(tema::px(ANCHO_COLUMNA_IZQUIERDA));
    auto* capaIzquierda = new QVBoxLayout(izquierda);
    capaIzquierda->setContentsMargins(0, 0, 0, 0);
    capaIzquierda->setSpacing(18);
    capaIzquierda->addWidget(tarjetaNodos());
    capaIzquierda->addWidget(tarjetaAristas());
    capaIzquierda->addWidget(tarjetaFiltros());
    capaIzquierda->addWidget(tarjetaResolver());
    capaIzquierda->addStretch(1);
    columnas->addWidget(izquierda, 0, Qt::AlignTop);

    auto* derecha = new QVBoxLayout;
    derecha->setSpacing(18);
    derecha->addWidget(tarjetaLienzo());
    resultado_ = new PanelResultado;
    derecha->addWidget(resultado_);
    derecha->addStretch(1);
    columnas->addLayout(derecha, 1);

    principal->addLayout(columnas);
    principal->addStretch(1);

    auto* scrollPagina = new QScrollArea;
    scrollPagina->setObjectName(QStringLiteral("scrollPagina"));
    scrollPagina->setWidget(pagina);
    scrollPagina->setWidgetResizable(true);
    scrollPagina->setFrameShape(QFrame::NoFrame);
    setCentralWidget(scrollPagina);
}

QFrame* VentanaPrincipal::tarjetaLienzo() {
    QVBoxLayout* c = nullptr;
    QFrame* marco = tarjeta(QStringLiteral("Grafo"), c);

    auto* barra = new QHBoxLayout;
    barra->setSpacing(10);
    barra->addWidget(pista(QStringLiteral("Clic en el fondo: nuevo nodo  ·  Arrastrar: mover  ·  "
                                          "Clic derecho: opciones  ·  Ctrl + rueda: zoom")), 1);
    auto* botonRepetir = new QPushButton(QStringLiteral("▶  Ver recorrido"));
    auto* botonAjustar = new QPushButton(QStringLiteral("Ajustar vista"));
    barra->addWidget(botonRepetir);
    barra->addWidget(botonAjustar);
    c->addLayout(barra);

    lienzo_ = new LienzoGrafo(&grafo_);
    lienzo_->setFixedHeight(ALTO_LIENZO);
    lienzo_->alHacerClicEnVacio = [this](QPointF p) { agregarNodo(p, true); };
    lienzo_->alPedirMenuNodo = [this](int i, QPoint p) { menuNodo(i, p); };
    lienzo_->alPedirMenuArista = [this](int i, QPoint p) { menuArista(i, p); };
    c->addWidget(lienzo_);

    // Referencias de colores
    auto* leyenda = new QHBoxLayout;
    leyenda->setSpacing(18);
    const auto punto = [](const QString& color, const QString& texto) {
        auto* l = new QLabel(QStringLiteral("<span style=\"color:%1;\">●</span>&nbsp; %2").arg(color, texto));
        l->setObjectName(QStringLiteral("pista"));
        return l;
    };
    leyenda->addWidget(punto(tema::camino().name(), QStringLiteral("camino")));
    leyenda->addWidget(punto(tema::obligatorio().name(), QStringLiteral("obligatorio")));
    leyenda->addWidget(punto(tema::bloqueado().name(), QStringLiteral("bloqueado (línea punteada)")));
    leyenda->addWidget(punto(tema::tenue().name(), QStringLiteral("filtrado (transparente)")));
    leyenda->addStretch(1);
    contadorNodos_ = pista(QString());
    contadorNodos_->setWordWrap(false);
    leyenda->addWidget(contadorNodos_);
    c->addLayout(leyenda);

    connect(botonRepetir, &QPushButton::clicked, this, [this]() { lienzo_->repetirAnimacion(); });
    connect(botonAjustar, &QPushButton::clicked, this, [this]() { lienzo_->ajustarVista(); });
    return marco;
}

QFrame* VentanaPrincipal::tarjetaNodos() {
    QVBoxLayout* c = nullptr;
    QFrame* marco = tarjeta(QStringLiteral("Nodos"), c);

    auto* formulario = new QGridLayout;
    formulario->setHorizontalSpacing(10);
    formulario->setVerticalSpacing(6);
    nombreNodo_ = new QLineEdit;
    nombreNodo_->setPlaceholderText(QStringLiteral("Vacío = siguiente número libre"));
    // Solo un número desde 1: el primer dígito no puede ser 0
    nombreNodo_->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("([1-9][0-9]{0,%1})?").arg(grafos::Grafo::LARGO_MAXIMO_NOMBRE - 1)), nombreNodo_));
    nombreNodo_->setMaxLength(grafos::Grafo::LARGO_MAXIMO_NOMBRE);
    nombreNodo_->setInputMethodHints(Qt::ImhDigitsOnly);
    auto* agregar = new QPushButton(QStringLiteral("Agregar"));
    agregar->setObjectName(QStringLiteral("principal"));
    formulario->addWidget(rotulo(QStringLiteral("NÚMERO DEL NODO")), 0, 0);
    formulario->addWidget(nombreNodo_, 1, 0);
    formulario->addWidget(agregar, 1, 1);
    formulario->setColumnStretch(0, 1);
    c->addLayout(formulario);

    tablaNodos_ = crearTabla({QStringLiteral("NODO"), QStringLiteral("BLOQ."), QStringLiteral("OBLIG."), QString()}, 0);
    c->addWidget(tablaNodos_);
    c->addWidget(pista(QStringLiteral("Cada nodo es un número desde 1 (hasta %1 cifras). Un nodo puede estar "
                                      "bloqueado u obligatorio, no las dos cosas.").arg(grafos::Grafo::LARGO_MAXIMO_NOMBRE)));

    connect(agregar, &QPushButton::clicked, this, [this]() { agregarNodo(QPointF(), false); });
    connect(nombreNodo_, &QLineEdit::returnPressed, this, [this]() { agregarNodo(QPointF(), false); });
    return marco;
}

QFrame* VentanaPrincipal::tarjetaAristas() {
    QVBoxLayout* c = nullptr;
    QFrame* marco = tarjeta(QStringLiteral("Aristas"), c);

    auto* formulario = new QGridLayout;
    formulario->setHorizontalSpacing(10);
    formulario->setVerticalSpacing(6);
    desde_ = new ComboSinRueda;
    hasta_ = new ComboSinRueda;
    tipoArista_ = new ComboSinRueda;
    tipoArista_->addItem(QStringLiteral("No dirigido"));
    tipoArista_->addItem(QStringLiteral("Dirigido"));
    formulario->addWidget(rotulo(QStringLiteral("TIPO DE GRAFO (vale para todas las aristas)")), 0, 0, 1, 3);
    formulario->addWidget(tipoArista_, 1, 0, 1, 3);
    formulario->addWidget(rotulo(QStringLiteral("DESDE")), 2, 0);
    formulario->addWidget(rotulo(QStringLiteral("HASTA")), 2, 1);
    formulario->addWidget(desde_, 3, 0);
    formulario->addWidget(hasta_, 3, 1);

    etiquetaIda_ = rotulo(QStringLiteral("PESO IDA"));
    etiquetaVuelta_ = rotulo(QStringLiteral("PESO VUELTA"));
    pesoIda_ = crearSpin(1);
    pesoVuelta_ = crearSpin(1);
    auto* agregar = new QPushButton(QStringLiteral("Agregar"));
    agregar->setObjectName(QStringLiteral("principal"));
    formulario->addWidget(etiquetaIda_, 4, 0);
    formulario->addWidget(etiquetaVuelta_, 4, 1);
    formulario->addWidget(pesoIda_, 5, 0);
    formulario->addWidget(pesoVuelta_, 5, 1);
    formulario->addWidget(agregar, 5, 2);
    formulario->setRowMinimumHeight(2, 26);
    formulario->setRowMinimumHeight(4, 26);
    formulario->setColumnStretch(0, 1);
    formulario->setColumnStretch(1, 1);
    formulario->setColumnStretch(2, 1);
    c->addLayout(formulario);

    tablaAristas_ = crearTabla({QStringLiteral("ARISTA"), QStringLiteral("IDA"), QStringLiteral("VUELTA"),
                                QStringLiteral("BLOQ."), QString()}, 0);
    c->addWidget(tablaAristas_);
    c->addWidget(pista(QStringLiteral("No dirigido: cada arista tiene peso de ida (desde → hasta) y de vuelta "
                                      "(hasta → desde). Dirigido: un solo peso por arista. Al cambiar el tipo, "
                                      "A — B pasa a A → B y B → A, y al volver se juntan de nuevo.")));

    // El tipo es del grafo entero: convierte todas las aristas
    connect(tipoArista_, &QComboBox::currentIndexChanged, this, [this](int indice) {
        guardarParaDeshacer();
        grafo_.setDirigido(indice == 1);
        refrescarTodo();
    });
    connect(agregar, &QPushButton::clicked, this, [this]() { agregarArista(); });
    return marco;
}

QFrame* VentanaPrincipal::tarjetaFiltros() {
    QVBoxLayout* c = nullptr;
    QFrame* marco = tarjeta(QStringLiteral("Filtros"), c);
    evitarPares_ = new QCheckBox(QStringLiteral("Evitar nodos pares  (nodo n.º 2, 4, 6…)"));
    evitarImpares_ = new QCheckBox(QStringLiteral("Evitar nodos impares  (nodo n.º 1, 3, 5…)"));
    evitarAristasPares_ = new QCheckBox(QStringLiteral("Evitar aristas de peso par  (2, 4, 6…)"));
    evitarAristasImpares_ = new QCheckBox(QStringLiteral("Evitar aristas de peso impar  (1, 3, 5…)"));
    c->addWidget(rotulo(QStringLiteral("NODOS")));
    c->addWidget(evitarPares_);
    c->addWidget(evitarImpares_);
    c->addWidget(rotulo(QStringLiteral("ARISTAS")));
    c->addWidget(evitarAristasPares_);
    c->addWidget(evitarAristasImpares_);
    soloPositivos_ = new QCheckBox(QStringLiteral("Solo pesos positivos  (el 0 no cuenta)"));
    soloNegativos_ = new QCheckBox(QStringLiteral("Solo pesos negativos  (el 0 no cuenta)"));
    c->addWidget(soloPositivos_);
    c->addWidget(soloNegativos_);

    auto* fila = new QGridLayout;
    fila->setHorizontalSpacing(10);
    fila->setVerticalSpacing(6);
    pesoABloquear_ = crearSpin(1);
    auto* bloquearPeso = new QPushButton(QStringLiteral("Bloquear peso"));
    fila->addWidget(rotulo(QStringLiteral("BLOQUEAR ARISTAS DE PESO")), 0, 0, 1, 2);
    fila->addWidget(pesoABloquear_, 1, 0);
    fila->addWidget(bloquearPeso, 1, 1);
    fila->setColumnStretch(0, 1);
    c->addLayout(fila);
    listaPesos_ = new QWidget;
    listaPesos_->setLayout(new QGridLayout);
    listaPesos_->layout()->setContentsMargins(0, 0, 0, 0);
    c->addWidget(listaPesos_);
    c->addWidget(pista(QStringLiteral("Los filtros de aristas (par, impar, positivo, negativo o un peso) cortan cada sentido que pesa eso: en "
                                      "una no dirigida con ida 4 y vuelta 3, evitar pares corta solo la ida. Para evitar un "
                                      "nodo o una arista puntual, tildá «Bloq.» en su tabla o hacé clic derecho en el dibujo.")));

    filtrosActivos_ = pista(QString());
    c->addWidget(filtrosActivos_);
    quitarFiltros_ = new QPushButton(QStringLiteral("Quitar filtros"));
    c->addWidget(quitarFiltros_);
    connect(quitarFiltros_, &QPushButton::clicked, this, [this]() { quitarFiltros(); });

    const auto alCambiar = [this](bool) {
        lienzo_->setFiltros(filtros());
        cambio();
    };
    connect(evitarPares_, &QCheckBox::toggled, this, alCambiar);
    connect(evitarImpares_, &QCheckBox::toggled, this, alCambiar);
    connect(evitarAristasPares_, &QCheckBox::toggled, this, alCambiar);
    connect(evitarAristasImpares_, &QCheckBox::toggled, this, alCambiar);
    // Solo positivos y solo negativos son excluyentes: tildar uno destilda el otro
    connect(soloPositivos_, &QCheckBox::toggled, this, [this](bool v) {
        if (v) {
            const QSignalBlocker b(soloNegativos_);
            soloNegativos_->setChecked(false);
        }
        lienzo_->setFiltros(filtros());
        cambio();
    });
    connect(soloNegativos_, &QCheckBox::toggled, this, [this](bool v) {
        if (v) {
            const QSignalBlocker b(soloPositivos_);
            soloPositivos_->setChecked(false);
        }
        lienzo_->setFiltros(filtros());
        cambio();
    });
    connect(bloquearPeso, &QPushButton::clicked, this, [this]() { agregarPesoBloqueado(); });
    refrescarPesosBloqueados();
    return marco;
}

QFrame* VentanaPrincipal::tarjetaResolver() {
    QVBoxLayout* c = nullptr;
    QFrame* marco = tarjeta(QStringLiteral("Resolver"), c);

    auto* formulario = new QGridLayout;
    formulario->setHorizontalSpacing(10);
    formulario->setVerticalSpacing(6);
    origen_ = new ComboSinRueda;
    destino_ = new ComboSinRueda;
    algoritmo_ = new ComboSinRueda;
    algoritmo_->addItem(QStringLiteral("Dijkstra"));
    algoritmo_->addItem(QStringLiteral("Bellman-Ford"));
    algoritmo_->addItem(QStringLiteral("Matriz de adyacencia"));
    formulario->addWidget(rotulo(QStringLiteral("ORIGEN")), 0, 0);
    formulario->addWidget(rotulo(QStringLiteral("DESTINO")), 0, 1);
    formulario->addWidget(origen_, 1, 0);
    formulario->addWidget(destino_, 1, 1);
    formulario->addWidget(rotulo(QStringLiteral("ALGORITMO")), 2, 0, 1, 2);
    formulario->addWidget(algoritmo_, 3, 0, 1, 2);
    formulario->setRowMinimumHeight(2, 26);
    formulario->setColumnStretch(0, 1);
    formulario->setColumnStretch(1, 1);
    c->addLayout(formulario);

    animar_ = new QCheckBox(QStringLiteral("Mostrar el recorrido paso a paso"));
    animar_->setChecked(true);
    c->addWidget(animar_);

    auto* calcularBoton = new QPushButton(QStringLiteral("Calcular camino"));
    calcularBoton->setObjectName(QStringLiteral("principal"));
    calcularBoton->setMinimumHeight(40);
    c->addWidget(calcularBoton);

    auto* fila = new QHBoxLayout;
    fila->setSpacing(10);
    deshacer_ = new QPushButton(QStringLiteral("Deshacer"));
    deshacer_->setEnabled(false);
    deshacer_->setToolTip(QStringLiteral("Deshace la última edición del grafo (Ctrl + Z)"));
    auto* ejemplo = new QPushButton(QStringLiteral("Cargar ejemplo"));
    auto* limpiar = new QPushButton(QStringLiteral("Limpiar todo"));
    limpiar->setObjectName(QStringLiteral("peligro"));
    fila->addWidget(deshacer_, 1);
    fila->addWidget(ejemplo, 1);
    fila->addWidget(limpiar, 1);
    c->addLayout(fila);
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
    connect(deshacer_, &QPushButton::clicked, this, [this]() { deshacer(); });
    connect(ejemplo, &QPushButton::clicked, this, [this]() { cargarEjemplo(); });
    connect(limpiar, &QPushButton::clicked, this, [this]() { limpiarTodo(); });
    return marco;
}

// ============================================================
//  Refresco
// ============================================================
void VentanaPrincipal::refrescarTodo() {
    sincronizarTipo();
    refrescarCombos();
    refrescarTablaNodos();
    refrescarTablaAristas();
    lienzo_->setFiltros(filtros());
    lienzo_->setOrigenDestino(origen_->currentIndex(), destino_->currentIndex());
    lienzo_->reconstruir();
    cambio();
}

// El selector de tipo y el formulario de aristas siguen al grafo
void VentanaPrincipal::sincronizarTipo() {
    const bool dirigido = grafo_.dirigido();
    {
        const QSignalBlocker b(tipoArista_);
        tipoArista_->setCurrentIndex(dirigido ? 1 : 0);
    }
    pesoVuelta_->setEnabled(!dirigido);
    etiquetaVuelta_->setEnabled(!dirigido);
    etiquetaIda_->setText(dirigido ? QStringLiteral("PESO") : QStringLiteral("PESO IDA"));
    tablaAristas_->setColumnHidden(2, dirigido);
    tablaAristas_->horizontalHeaderItem(1)->setText(dirigido ? QStringLiteral("PESO") : QStringLiteral("IDA"));
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
        // El filtro par/impar usa la posición del nodo (n.º), que puede no coincidir con su nombre
        const QString nombre = aQString(nodo.nombre);
        const QString posicion = QString::number(i + 1);
        tablaNodos_->setItem(i, 0, itemTexto(nombre == posicion ? nombre
                                                                : nombre + QStringLiteral("   (n.º ") + posicion + QStringLiteral(")")));

        auto* bloqueado = new QCheckBox;
        auto* obligatorio = new QCheckBox;
        bloqueado->setChecked(nodo.bloqueado);
        obligatorio->setChecked(nodo.obligatorio);
        // Uno u otro: tildar uno destilda el otro
        connect(bloqueado, &QCheckBox::toggled, this, [this, i, obligatorio](bool v) {
            guardarParaDeshacer();
            grafo_.setBloqueado(i, v);
            if (v) {
                const QSignalBlocker b(obligatorio);
                obligatorio->setChecked(false);
            }
            lienzo_->setFiltros(filtros());
            cambio();
        });
        connect(obligatorio, &QCheckBox::toggled, this, [this, i, bloqueado](bool v) {
            guardarParaDeshacer();
            grafo_.setObligatorio(i, v);
            if (v) {
                const QSignalBlocker b(bloqueado);
                bloqueado->setChecked(false);
            }
            lienzo_->setFiltros(filtros());
            cambio();
        });
        tablaNodos_->setCellWidget(i, 1, centrado(bloqueado));
        tablaNodos_->setCellWidget(i, 2, centrado(obligatorio));

        auto* quitar = botonQuitar(QStringLiteral("Quitar nodo"));
        // Diferido: no se puede borrar la tabla mientras se procesa el clic de su botón
        connect(quitar, &QToolButton::clicked, this, [this, i]() {
            QTimer::singleShot(0, this, [this, i]() { eliminarNodo(i); });
        });
        tablaNodos_->setCellWidget(i, 3, centrado(quitar));
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
        connect(ida, &QSpinBox::valueChanged, this, [this, k](int v) {
            guardarParaDeshacer();
            grafos::Arista& a = grafo_.arista(k);
            a.pesoIda = v;
            if (a.dirigida) a.pesoVuelta = v;
            lienzo_->reconstruir();
            cambio();
        });
        tablaAristas_->setCellWidget(k, 1, centrado(ida));

        if (e.dirigida) {
            auto* guion = itemTexto(QStringLiteral("—"));
            guion->setTextAlignment(Qt::Alignment(Qt::AlignCenter));
            guion->setForeground(tema::tenue());
            tablaAristas_->setItem(k, 2, guion);
        } else {
            auto* vuelta = spinDeTabla(e.pesoVuelta);
            connect(vuelta, &QSpinBox::valueChanged, this, [this, k](int v) {
                guardarParaDeshacer();
                grafo_.arista(k).pesoVuelta = v;
                lienzo_->reconstruir();
                cambio();
            });
            tablaAristas_->setCellWidget(k, 2, centrado(vuelta));
        }

        auto* bloqueada = new QCheckBox;
        bloqueada->setChecked(e.bloqueada);
        connect(bloqueada, &QCheckBox::toggled, this, [this, k](bool v) {
            guardarParaDeshacer();
            grafo_.arista(k).bloqueada = v;
            lienzo_->setFiltros(filtros());
            cambio();
        });
        tablaAristas_->setCellWidget(k, 3, centrado(bloqueada));

        auto* quitar = botonQuitar(QStringLiteral("Quitar arista"));
        connect(quitar, &QToolButton::clicked, this, [this, k]() {
            QTimer::singleShot(0, this, [this, k]() { eliminarArista(k); });
        });
        tablaAristas_->setCellWidget(k, 4, centrado(quitar));
    });
    ajustarAltoTabla(tablaAristas_);
}

// Después del primer cálculo, cualquier cambio vuelve a calcular (como el prototipo).
void VentanaPrincipal::cambio() {
    actualizarEstado();
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
    guardarParaDeshacer();
    grafo_.agregarNodo(nombre, libre.x(), libre.y());
    nombreNodo_->clear();
    refrescarTodo();
}

void VentanaPrincipal::agregarArista() {
    pesoIda_->interpretText(); // toma lo tipeado aunque el campo siga con el foco
    pesoVuelta_->interpretText();
    const int a = desde_->currentIndex();
    const int b = hasta_->currentIndex();
    const bool dirigida = grafo_.dirigido();
    const std::string error = grafo_.validarArista(a, b, dirigida);
    if (!error.empty()) {
        QMessageBox::warning(this, QStringLiteral("No se puede agregar la arista"), aQString(error));
        return;
    }
    guardarParaDeshacer();
    grafo_.agregarArista(a, b, pesoIda_->value(), pesoVuelta_->value(), dirigida);
    refrescarTodo();
}

void VentanaPrincipal::eliminarNodo(int indice) {
    guardarParaDeshacer();
    grafo_.eliminarNodo(indice);
    refrescarTodo();
}

void VentanaPrincipal::eliminarArista(int indice) {
    guardarParaDeshacer();
    grafo_.eliminarArista(indice);
    refrescarTodo();
}

grafos::Filtros VentanaPrincipal::filtros() const {
    grafos::Filtros f;
    f.evitarPares = evitarPares_->isChecked();
    f.evitarImpares = evitarImpares_->isChecked();
    f.pesosBloqueados = pesosBloqueados_;
    f.evitarAristasPares = evitarAristasPares_->isChecked();
    f.evitarAristasImpares = evitarAristasImpares_->isChecked();
    f.soloPositivos = soloPositivos_->isChecked();
    f.soloNegativos = soloNegativos_->isChecked();
    return f;
}

// ---- pesos bloqueados: una lista de botones "peso 4 ✕" ----
void VentanaPrincipal::refrescarPesosBloqueados() {
    auto* grilla = static_cast<QGridLayout*>(listaPesos_->layout());
    // Borra los botones anteriores (recursivo, sin while)
    std::function<void()> vaciar = [&]() {
        QLayoutItem* item = grilla->takeAt(0);
        if (item == nullptr) return;
        if (item->widget() != nullptr) item->widget()->deleteLater();
        delete item;
        vaciar();
    };
    vaciar();
    const int POR_FILA = 4;
    recorrer(0, static_cast<int>(pesosBloqueados_.size()), [&](int i) {
        const int peso = pesosBloqueados_[i];
        auto* chip = new QToolButton;
        chip->setObjectName(QStringLiteral("chipPeso"));
        chip->setText(QStringLiteral("peso %1  ✕").arg(peso));
        chip->setToolTip(QStringLiteral("Quitar el bloqueo del peso %1").arg(peso));
        connect(chip, &QToolButton::clicked, this, [this, peso]() {
            QTimer::singleShot(0, this, [this, peso]() { quitarPesoBloqueado(peso); });
        });
        grilla->addWidget(chip, i / POR_FILA, i % POR_FILA);
    });
    grilla->setColumnStretch(POR_FILA, 1);
    listaPesos_->setVisible(!pesosBloqueados_.empty());
}

void VentanaPrincipal::agregarPesoBloqueado() {
    pesoABloquear_->interpretText(); // toma lo tipeado aunque no se haya salido del campo
    const int peso = pesoABloquear_->value();
    if (grafos::pesoBloqueado(filtros(), peso)) {
        QMessageBox::information(this, QStringLiteral("Peso ya bloqueado"),
                                 QStringLiteral("El peso %1 ya está bloqueado.").arg(peso));
        return;
    }
    pesosBloqueados_.push_back(peso);
    refrescarPesosBloqueados();
    lienzo_->setFiltros(filtros());
    cambio();
}

void VentanaPrincipal::quitarPesoBloqueado(int peso) {
    std::vector<int> quedan;
    recorrer(0, static_cast<int>(pesosBloqueados_.size()), [&](int i) {
        if (pesosBloqueados_[i] != peso) quedan.push_back(pesosBloqueados_[i]);
    });
    pesosBloqueados_ = quedan;
    refrescarPesosBloqueados();
    lienzo_->setFiltros(filtros());
    cambio();
}

void VentanaPrincipal::limpiarFiltros() {
    const QSignalBlocker b1(evitarPares_);
    const QSignalBlocker b2(evitarImpares_);
    const QSignalBlocker b3(evitarAristasPares_);
    const QSignalBlocker b4(evitarAristasImpares_);
    const QSignalBlocker b5(soloPositivos_);
    const QSignalBlocker b6(soloNegativos_);
    evitarPares_->setChecked(false);
    evitarImpares_->setChecked(false);
    evitarAristasPares_->setChecked(false);
    evitarAristasImpares_->setChecked(false);
    soloPositivos_->setChecked(false);
    soloNegativos_->setChecked(false);
    pesosBloqueados_.clear();
    refrescarPesosBloqueados();
}

// "Quitar filtros": apaga todos los filtros y desbloquea los nodos y aristas que estaban bloqueados
void VentanaPrincipal::quitarFiltros() {
    limpiarFiltros();
    recorrer(0, grafo_.cantidadNodos(), [&](int i) { grafo_.setBloqueado(i, false); });
    recorrer(0, grafo_.cantidadAristas(), [&](int k) { grafo_.arista(k).bloqueada = false; });
    refrescarTodo(); // vuelve a dibujar las tablas con las casillas «Bloq.» destildadas
}

// Contador de nodos y resumen de filtros activos (se llama en cada cambio)
void VentanaPrincipal::actualizarEstado() {
    const int n = grafo_.cantidadNodos();
    contadorNodos_->setText(n == 1 ? QStringLiteral("1 nodo en el lienzo")
                                   : QStringLiteral("%1 nodos en el lienzo").arg(n));

    QStringList activos;
    if (evitarPares_->isChecked()) activos << QStringLiteral("nodos pares");
    if (evitarImpares_->isChecked()) activos << QStringLiteral("nodos impares");
    if (evitarAristasPares_->isChecked()) activos << QStringLiteral("aristas pares");
    if (evitarAristasImpares_->isChecked()) activos << QStringLiteral("aristas impares");
    if (soloPositivos_->isChecked()) activos << QStringLiteral("solo positivos");
    if (soloNegativos_->isChecked()) activos << QStringLiteral("solo negativos");
    recorrer(0, static_cast<int>(pesosBloqueados_.size()),
             [&](int i) { activos << QStringLiteral("peso %1").arg(pesosBloqueados_[i]); });
    recorrer(0, n, [&](int i) {
        if (grafo_.nodo(i).bloqueado) activos << QStringLiteral("nodo ") + aQString(grafo_.nodo(i).nombre);
    });
    recorrer(0, grafo_.cantidadAristas(), [&](int k) {
        if (grafo_.arista(k).bloqueada) activos << QStringLiteral("arista ") + aQString(grafo_.describirArista(k));
    });
    filtrosActivos_->setText(activos.isEmpty() ? QStringLiteral("Filtros activos: ninguno")
                                               : QStringLiteral("Filtros activos: ") + activos.join(QStringLiteral("  ·  ")));
    quitarFiltros_->setEnabled(!activos.isEmpty());
}

// ---- Deshacer ----
void VentanaPrincipal::guardarParaDeshacer() {
    Estado e;
    e.grafo = grafo_;
    e.origen = origen_->currentIndex();
    e.destino = destino_->currentIndex();
    historial_.push_back(e);
    if (static_cast<int>(historial_.size()) > MAXIMO_DESHACER) historial_.erase(historial_.begin());
    deshacer_->setEnabled(true);
}

void VentanaPrincipal::deshacer() {
    if (historial_.empty()) return;
    const Estado e = historial_.back();
    historial_.pop_back();
    grafo_ = e.grafo;
    if (grafo_.cantidadNodos() == 0) {
        calculado_ = false;
        resultado_->mostrarVacio();
    }
    refrescarTodo();
    {
        const QSignalBlocker b1(origen_);
        const QSignalBlocker b2(destino_);
        if (e.origen >= 0 && e.origen < origen_->count()) origen_->setCurrentIndex(e.origen);
        if (e.destino >= 0 && e.destino < destino_->count()) destino_->setCurrentIndex(e.destino);
    }
    lienzo_->setOrigenDestino(origen_->currentIndex(), destino_->currentIndex());
    cambio();
    deshacer_->setEnabled(!historial_.empty());
}

grafos::Algoritmo VentanaPrincipal::algoritmoElegido() const {
    if (algoritmo_->currentIndex() == 1) return grafos::Algoritmo::BellmanFord;
    if (algoritmo_->currentIndex() == 2) return grafos::Algoritmo::MatrizAdyacencia;
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
    guardarParaDeshacer();
    grafo_.cargarEjemplo(origen, destino);
    limpiarFiltros();
    sincronizarTipo();
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
    guardarParaDeshacer();
    grafo_.limpiar();
    calculado_ = false;
    limpiarFiltros();
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
    if (elegida == bloquear || elegida == obligatorio) guardarParaDeshacer();
    if (elegida == bloquear) grafo_.setBloqueado(indice, !nodo.bloqueado);
    if (elegida == obligatorio) grafo_.setObligatorio(indice, !nodo.obligatorio);
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
    if (elegida == bloquear) guardarParaDeshacer();
    if (elegida == bloquear) grafo_.arista(indice).bloqueada = !arista.bloqueada;
    const bool borrar = elegida == eliminar;
    QTimer::singleShot(0, this, [this, indice, borrar]() {
        if (borrar) eliminarArista(indice);
        else refrescarTodo();
    });
}
