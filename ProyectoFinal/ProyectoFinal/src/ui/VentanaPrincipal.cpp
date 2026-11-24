#include "VentanaPrincipal.h"
#include "Controles.h"
#include "ItemsGrafo.h"
#include "LienzoGrafo.h"
#include "PanelResultado.h"
#include "Tema.h"
#include "core/Recorrer.h"

#include <QAbstractItemView>
#include <QCloseEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QScreen>
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
const int NODOS_PARA_DIFERIR_CALCULO = 20; // con más nodos, el recálculo espera a que terminen los cambios seguidos
const int DEMORA_RECALCULO_MS = 80;

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
    t->setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::DoubleClicked); // solo las celdas de peso son editables
    t->setSelectionMode(QAbstractItemView::NoSelection);
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

QTableWidgetItem* itemTilde(bool marcado) {
    auto* i = new QTableWidgetItem;
    i->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    i->setCheckState(marcado ? Qt::Checked : Qt::Unchecked);
    i->setTextAlignment(Qt::AlignCenter);
    return i;
}

QTableWidgetItem* itemQuitar(const QString& ayuda) {
    auto* i = new QTableWidgetItem(QStringLiteral("✕"));
    i->setFlags(Qt::ItemIsEnabled);
    i->setTextAlignment(Qt::AlignCenter);
    i->setForeground(tema::tenue());
    i->setToolTip(ayuda);
    return i;
}

QTableWidgetItem* itemPeso(int valor) {
    auto* i = new QTableWidgetItem;
    i->setData(Qt::EditRole, valor);
    i->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
    i->setTextAlignment(Qt::AlignCenter);
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

bool confirmar(QWidget* padre, const QString& titulo, const QString& texto) {
    return QMessageBox::question(padre, titulo, texto) == QMessageBox::Yes;
}

// Rellena un combo con los nombres de los nodos, conservando la selección si se puede.
// Con `sinSeleccionSiDesaparece`, si el nodo elegido ya no existe el combo queda vacío en vez de saltar
// a otro nodo (así no cambia el origen o el destino sin que se note).
void llenarCombo(QComboBox* combo, const grafos::Grafo& g, int indicePorDefecto, bool sinSeleccionSiDesaparece = false) {
    const QSignalBlocker bloqueo(combo);
    const QString anterior = combo->currentText();
    combo->clear();
    recorrer(0, g.cantidadNodos(), [&](int i) { combo->addItem(aQString(g.nodo(i).nombre)); });
    const int encontrado = combo->findText(anterior);
    if (encontrado >= 0) combo->setCurrentIndex(encontrado);
    else if (sinSeleccionSiDesaparece && !anterior.isEmpty()) combo->setCurrentIndex(-1);
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
    if (QWidget* w = QApplication::focusWidget()) w->clearFocus(); // confirma un peso a medio escribir
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
    capaIzquierda->addStretch(1);
    columnas->addWidget(izquierda, 0, Qt::AlignTop);

    auto* derecha = new QVBoxLayout;
    derecha->setSpacing(18);
    derecha->addWidget(tarjetaLienzo());
    derecha->addWidget(tarjetaResolver()); // justo debajo del dibujo y arriba del resultado: no hace falta scrollear
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
    barra->addWidget(pista(QStringLiteral("Doble clic en el fondo: nuevo nodo  ·  Arrastrar: mover  ·  "
                                          "Clic derecho: opciones  ·  Ctrl + rueda: zoom")), 1);
    auto* botonRepetir = new QPushButton(QStringLiteral("▶  Ver recorrido"));
    auto* botonAjustar = new QPushButton(QStringLiteral("Ajustar vista"));
    barra->addWidget(botonRepetir);
    barra->addWidget(botonAjustar);
    c->addLayout(barra);

    lienzo_ = new LienzoGrafo(&grafo_);
    lienzo_->setFixedHeight(ALTO_LIENZO);
    lienzo_->alHacerClicEnVacio = [this](QPointF p) { agregarNodo(p, true); };
    lienzo_->alEmpezarArrastre = [this]() { guardarParaDeshacer(); }; // mover un nodo también se deshace
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
    // Solo dígitos. Se deja tipear (o pegar) de más y el largo, el 0 inicial y los repetidos se avisan al
    // agregar: así no se recorta nada en silencio ni se traba el borrado de un dígito.
    nombreNodo_->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[0-9]{0,12}")), nombreNodo_));
    nombreNodo_->setMaxLength(12);
    nombreNodo_->installEventFilter(this);
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

    // Una sola conexión para toda la tabla: la fila se lee del propio ítem (no hay lambdas por fila)
    connect(tablaNodos_, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* it) {
        const int i = it->row();
        if (i < 0 || i >= grafo_.cantidadNodos() || (it->column() != 1 && it->column() != 2)) return;
        const bool v = it->checkState() == Qt::Checked;
        if (it->column() == 2 && v && !grafo_.nodo(i).obligatorio && cantidadObligatorios(0) >= grafos::MAXIMO_OBLIGATORIOS) {
            { const QSignalBlocker b(tablaNodos_); it->setCheckState(Qt::Unchecked); }
            QMessageBox::warning(this, QStringLiteral("Demasiados obligatorios"),
                                 QStringLiteral("Como máximo puede haber %1 nodos obligatorios (probar todos los órdenes "
                                                "sería demasiado lento).").arg(grafos::MAXIMO_OBLIGATORIOS));
            return;
        }
        guardarParaDeshacer();
        if (it->column() == 1) grafo_.setBloqueado(i, v); else grafo_.setObligatorio(i, v);
        // Bloqueado y obligatorio se excluyen: se refleja en la otra casilla
        const QSignalBlocker b(tablaNodos_);
        tablaNodos_->item(i, 1)->setCheckState(grafo_.nodo(i).bloqueado ? Qt::Checked : Qt::Unchecked);
        tablaNodos_->item(i, 2)->setCheckState(grafo_.nodo(i).obligatorio ? Qt::Checked : Qt::Unchecked);
        lienzo_->setFiltros(filtros());
        cambio();
    });
    connect(tablaNodos_, &QTableWidget::cellClicked, this, [this](int fila, int columna) {
        if (columna == 3) quitarFilaDiferido(true, fila);
    });
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
                                      "(hasta → desde). Dirigido: un solo peso por arista. Al pasar a dirigido, "
                                      "A — B queda solo como A → B y se pierde el peso de vuelta; «Deshacer» lo recupera.")));

    // El tipo es del grafo entero: convierte todas las aristas
    connect(tipoArista_, &QComboBox::currentIndexChanged, this, [this](int indice) {
        const bool haciaDirigido = indice == 1;
        const int pierde = grafo_.aristasQuePierdenDatos(haciaDirigido);
        if (pierde > 0) {
            const QString que = haciaDirigido
                ? QStringLiteral("%1 arista(s) tienen un peso de vuelta distinto del de ida: ese peso de vuelta se va a descartar.").arg(pierde)
                : QStringLiteral("%1 par(es) de aristas opuestas se juntan en una sola y solo una de las dos estaba bloqueada: el bloqueo va a valer para las dos.").arg(pierde);
            if (!confirmar(this, QStringLiteral("Cambiar el tipo de grafo"),
                           que + QStringLiteral("\n\nPodés recuperarlo con «Deshacer» mientras no hagas muchos cambios más. ¿Seguir?"))) {
                sincronizarTipo(); // vuelve el selector a como estaba
                return;
            }
        }
        guardarParaDeshacer();
        grafo_.setDirigido(indice == 1);
        refrescarTodo();
    });
    connect(agregar, &QPushButton::clicked, this, [this]() { agregarArista(); });

    tablaAristas_->setItemDelegateForColumn(1, new DelegadoPeso(tablaAristas_));
    tablaAristas_->setItemDelegateForColumn(2, new DelegadoPeso(tablaAristas_));
    connect(tablaAristas_, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* it) {
        const int k = it->row();
        const int c = it->column();
        if (k < 0 || k >= grafo_.cantidadAristas() || c < 1 || c > 3) return;
        grafos::Arista& a = grafo_.arista(k);
        if (c == 3) {
            guardarParaDeshacer();
            a.bloqueada = it->checkState() == Qt::Checked;
            lienzo_->setFiltros(filtros());
            cambio();
            return;
        }
        const int v = it->data(Qt::EditRole).toInt();
        if ((c == 1 && v == a.pesoIda) || (c == 2 && v == a.pesoVuelta)) return;
        guardarParaDeshacer();
        if (c == 1) { a.pesoIda = v; if (a.dirigida) a.pesoVuelta = v; }
        else a.pesoVuelta = v;
        lienzo_->reconstruir();
        cambio();
    });
    connect(tablaAristas_, &QTableWidget::cellClicked, this, [this](int fila, int columna) {
        if (columna == 4) quitarFilaDiferido(false, fila);
    });
    return marco;
}

QFrame* VentanaPrincipal::tarjetaFiltros() {
    QVBoxLayout* c = nullptr;
    QFrame* marco = tarjeta(QStringLiteral("Filtros"), c);
    evitarAristasPares_ = new QCheckBox(QStringLiteral("Evitar aristas de peso par  (2, 4, 6…)"));
    evitarAristasImpares_ = new QCheckBox(QStringLiteral("Evitar aristas de peso impar  (1, 3, 5…)"));
    c->addWidget(rotulo(QStringLiteral("ARISTAS")));
    c->addWidget(evitarAristasPares_);
    c->addWidget(evitarAristasImpares_);
    c->addWidget(pista(QStringLiteral("Los filtros de aristas cortan cada sentido que pesa par o impar: en una no dirigida "
                                      "con ida 4 y vuelta 3, evitar pares corta solo la ida. Para no pasar por un nodo o una "
                                      "arista puntual, tildá «Bloq.» en su tabla o hacé clic derecho en el dibujo.")));

    filtrosActivos_ = pista(QString());
    c->addWidget(filtrosActivos_);
    quitarFiltros_ = new QPushButton(QStringLiteral("Quitar filtros"));
    c->addWidget(quitarFiltros_);
    connect(quitarFiltros_, &QPushButton::clicked, this, [this]() { quitarFiltros(); });

    connect(evitarAristasPares_, &QCheckBox::toggled, this,
            [this](bool marcado) { guardarFiltrosYCambiar(!marcado, evitarAristasImpares_->isChecked()); });
    connect(evitarAristasImpares_, &QCheckBox::toggled, this,
            [this](bool marcado) { guardarFiltrosYCambiar(evitarAristasPares_->isChecked(), !marcado); });
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
    algoritmo_->addItem(QStringLiteral("Matriz de adyacencia (Floyd-Warshall)"));
    // Tarjeta ancha (columna derecha): origen, destino y algoritmo en una sola fila
    formulario->addWidget(rotulo(QStringLiteral("ORIGEN")), 0, 0);
    formulario->addWidget(rotulo(QStringLiteral("DESTINO")), 0, 1);
    formulario->addWidget(rotulo(QStringLiteral("ALGORITMO")), 0, 2);
    formulario->addWidget(origen_, 1, 0);
    formulario->addWidget(destino_, 1, 1);
    formulario->addWidget(algoritmo_, 1, 2);
    formulario->setColumnStretch(0, 1);
    formulario->setColumnStretch(1, 1);
    formulario->setColumnStretch(2, 2);
    c->addLayout(formulario);

    animar_ = new QCheckBox(QStringLiteral("Mostrar el recorrido paso a paso"));
    animar_->setChecked(true);
    c->addWidget(animar_);

    auto* calcularBoton = new QPushButton(QStringLiteral("Calcular camino"));
    calcularBoton->setObjectName(QStringLiteral("principal"));
    calcularBoton->setMinimumHeight(tema::px(40));

    auto* fila = new QHBoxLayout;
    fila->setSpacing(10);
    fila->addWidget(calcularBoton, 3);
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
    c->addWidget(pista(QStringLiteral("Los nodos obligatorios se visitan en el orden que dé menor costo (hasta %1). "
                                      "Si hay dos caminos con el mismo costo, cada algoritmo puede mostrar uno distinto: los dos son correctos.")
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
    llenarCombo(origen_, grafo_, 0, true);
    llenarCombo(destino_, grafo_, n - 1, true);
}

void VentanaPrincipal::refrescarTablaNodos() {
    const int n = grafo_.cantidadNodos();
    const QSignalBlocker bloqueo(tablaNodos_); // llenar la tabla no cuenta como un cambio del usuario
    tablaNodos_->setUpdatesEnabled(false);
    tablaNodos_->setRowCount(0);
    tablaNodos_->setRowCount(n);
    recorrer(0, n, [&](int i) {
        const grafos::Nodo& nodo = grafo_.nodo(i);
        auto* texto = itemTexto(aQString(nodo.nombre));
        texto->setFlags(Qt::ItemIsEnabled);
        tablaNodos_->setItem(i, 0, texto);
        tablaNodos_->setItem(i, 1, itemTilde(nodo.bloqueado));
        tablaNodos_->setItem(i, 2, itemTilde(nodo.obligatorio));
        tablaNodos_->setItem(i, 3, itemQuitar(QStringLiteral("Quitar nodo")));
    });
    ajustarAltoTabla(tablaNodos_);
    tablaNodos_->setUpdatesEnabled(true);
}

void VentanaPrincipal::refrescarTablaAristas() {
    const int m = grafo_.cantidadAristas();
    const QSignalBlocker bloqueo(tablaAristas_);
    tablaAristas_->setUpdatesEnabled(false);
    tablaAristas_->setRowCount(0);
    tablaAristas_->setRowCount(m);
    recorrer(0, m, [&](int k) {
        const grafos::Arista& e = grafo_.arista(k);
        auto* texto = itemTexto(aQString(grafo_.describirArista(k)));
        texto->setFlags(Qt::ItemIsEnabled);
        tablaAristas_->setItem(k, 0, texto);
        tablaAristas_->setItem(k, 1, itemPeso(e.pesoIda));
        if (e.dirigida) {
            auto* guion = itemTexto(QStringLiteral("—"));
            guion->setFlags(Qt::ItemIsEnabled);
            guion->setTextAlignment(Qt::Alignment(Qt::AlignCenter));
            guion->setForeground(tema::tenue());
            tablaAristas_->setItem(k, 2, guion);
        } else {
            tablaAristas_->setItem(k, 2, itemPeso(e.pesoVuelta));
        }
        tablaAristas_->setItem(k, 3, itemTilde(e.bloqueada));
        tablaAristas_->setItem(k, 4, itemQuitar(QStringLiteral("Quitar arista")));
    });
    ajustarAltoTabla(tablaAristas_);
    tablaAristas_->setUpdatesEnabled(true);
}

// "✕" de una fila. Diferido (no se puede reconstruir la tabla dentro de su propio clic) y con un
// breve bloqueo: un doble clic accidental ya no borra también la fila que sube a ese lugar.
void VentanaPrincipal::quitarFilaDiferido(bool esNodo, int fila) {
    if (quitandoFila_) return;
    quitandoFila_ = true;
    // Si había un peso a medio escribir, se confirma ahora (antes de que las filas se corran de lugar)
    if (QWidget* w = QApplication::focusWidget()) w->clearFocus();
    QTimer::singleShot(0, this, [this, esNodo, fila]() {
        if (esNodo) eliminarNodo(fila); else eliminarArista(fila);
        QTimer::singleShot(350, this, [this]() { quitandoFila_ = false; });
    });
}

// Después del primer cálculo, cualquier cambio vuelve a calcular (como el prototipo).
void VentanaPrincipal::cambio() {
    actualizarEstado();
    if (calculado_) {
        // Con muchos nodos el resultado (sobre todo la matriz) tarda en armarse: si llegan varios
        // cambios seguidos (por ejemplo Enter repetido) se recalcula una sola vez al final.
        if (grafo_.cantidadNodos() > NODOS_PARA_DIFERIR_CALCULO) diferirCalculo();
        else calcular(false);
        return;
    }
    lienzo_->setResultado(nullptr, false);
}

void VentanaPrincipal::diferirCalculo() {
    if (recalculo_ == nullptr) {
        recalculo_ = new QTimer(this);
        recalculo_->setSingleShot(true);
        connect(recalculo_, &QTimer::timeout, this, [this]() {
            if (calculado_) calcular(false);
        });
    }
    recalculo_->start(DEMORA_RECALCULO_MS); // cada cambio nuevo reinicia la espera
}

// ============================================================
//  Acciones
// ============================================================
void VentanaPrincipal::agregarNodo(QPointF posicion, bool posicionElegida) {
    // Un clic en el dibujo usa siempre el siguiente número libre (no el que haya tipeado en el campo)
    const QString escrito = posicionElegida ? QString() : nombreNodo_->text().trimmed();
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
    if (!posicionElegida) nombreNodo_->clear();
    refrescarTodo();
    lienzo_->asegurarVisible(libre); // si no entró en la vista, se desplaza hasta el nodo nuevo
}

void VentanaPrincipal::agregarArista() {
    const auto vacio = [](const QSpinBox* s) {
        const QString t = s->text().trimmed();
        return t.isEmpty() || t == QStringLiteral("-");
    };
    if (vacio(pesoIda_) || (!grafo_.dirigido() && vacio(pesoVuelta_))) {
        QMessageBox::warning(this, QStringLiteral("No se puede agregar la arista"),
                             QStringLiteral("Escribí un peso (un número entero entre -99999 y 99999)."));
        return;
    }
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
    f.evitarAristasPares = evitarAristasPares_->isChecked();
    f.evitarAristasImpares = evitarAristasImpares_->isChecked();
    return f;
}

// Tildar o destildar un filtro: se guarda el estado anterior (el filtro ya cambió, así que se
// guarda con el valor previo) para que Ctrl + Z lo deshaga.
void VentanaPrincipal::guardarFiltrosYCambiar(bool paresAntes, bool imparesAntes) {
    Estado e;
    e.grafo = grafo_;
    e.origen = origen_->currentIndex();
    e.destino = destino_->currentIndex();
    e.evitarPares = paresAntes;
    e.evitarImpares = imparesAntes;
    historial_.push_back(e);
    if (static_cast<int>(historial_.size()) > MAXIMO_DESHACER) historial_.erase(historial_.begin());
    deshacer_->setEnabled(true);
    lienzo_->setFiltros(filtros());
    cambio();
}

int VentanaPrincipal::cantidadObligatorios(int i) const {
    if (i >= grafo_.cantidadNodos()) return 0;
    return (grafo_.nodo(i).obligatorio ? 1 : 0) + cantidadObligatorios(i + 1);
}

void VentanaPrincipal::limpiarFiltros() {
    const QSignalBlocker b1(evitarAristasPares_);
    const QSignalBlocker b2(evitarAristasImpares_);
    evitarAristasPares_->setChecked(false);
    evitarAristasImpares_->setChecked(false);
}

// "Quitar filtros": apaga todos los filtros y desbloquea los nodos y aristas que estaban bloqueados
void VentanaPrincipal::quitarFiltros() {
    guardarParaDeshacer(); // también se deshace
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
    if (evitarAristasPares_->isChecked()) activos << QStringLiteral("aristas pares");
    if (evitarAristasImpares_->isChecked()) activos << QStringLiteral("aristas impares");
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
    e.evitarPares = evitarAristasPares_->isChecked();
    e.evitarImpares = evitarAristasImpares_->isChecked();
    historial_.push_back(e);
    if (static_cast<int>(historial_.size()) > MAXIMO_DESHACER) historial_.erase(historial_.begin());
    deshacer_->setEnabled(true);
}

void VentanaPrincipal::deshacer() {
    if (historial_.empty()) return;
    if (QApplication::mouseButtons() & Qt::LeftButton) return; // no se deshace a mitad de un arrastre
    const Estado e = historial_.back();
    historial_.pop_back();
    grafo_ = e.grafo;
    {
        const QSignalBlocker b1(evitarAristasPares_);
        const QSignalBlocker b2(evitarAristasImpares_);
        evitarAristasPares_->setChecked(e.evitarPares);
        evitarAristasImpares_->setChecked(e.evitarImpares);
    }
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
    if (recalculo_ != nullptr) recalculo_->stop(); // ya se calcula ahora: no repetir
    const grafos::Resultado r = grafos::resolver(grafo_, origen_->currentIndex(), destino_->currentIndex(),
                                                 algoritmoElegido(), filtros());
    resultado_->mostrar(grafo_, r);
    lienzo_->setResultado(&r, animar && animar_->isChecked());
}

void VentanaPrincipal::cargarEjemplo() {
    int origen = 0;
    int destino = 0;
    if (grafo_.cantidadNodos() > 0 &&
        !confirmar(this, QStringLiteral("Cargar ejemplo"),
                   QStringLiteral("Se va a reemplazar el grafo actual por el ejemplo (podés volver con «Deshacer»). ¿Seguir?")))
        return;
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
    actualizarEstado(); // contador de nodos y filtros activos
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
    if (elegida == obligatorio && !nodo.obligatorio && cantidadObligatorios(0) >= grafos::MAXIMO_OBLIGATORIOS) {
        QMessageBox::warning(this, QStringLiteral("Demasiados obligatorios"),
                             QStringLiteral("Como máximo puede haber %1 nodos obligatorios.").arg(grafos::MAXIMO_OBLIGATORIOS));
        return;
    }
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

// ============================================================
//  Eventos de la ventana
// ============================================================
bool VentanaPrincipal::eventFilter(QObject* objeto, QEvent* evento) {
    // Mantener apretado Enter en "Número del nodo" repite la tecla decenas de veces por segundo y crearía
    // cientos de nodos: solo cuenta la primera pulsación.
    if (objeto == nombreNodo_ && evento->type() == QEvent::KeyPress) {
        const auto* tecla = static_cast<QKeyEvent*>(evento);
        if ((tecla->key() == Qt::Key_Return || tecla->key() == Qt::Key_Enter) && tecla->isAutoRepeat()) return true;
    }
    return QMainWindow::eventFilter(objeto, evento);
}

void VentanaPrincipal::closeEvent(QCloseEvent* evento) {
    if (grafo_.cantidadNodos() > 0 &&
        !confirmar(this, QStringLiteral("Cerrar"), QStringLiteral("Hay un grafo armado y no se guarda. ¿Cerrar igual?"))) {
        evento->ignore();
        return;
    }
    QMainWindow::closeEvent(evento);
}
