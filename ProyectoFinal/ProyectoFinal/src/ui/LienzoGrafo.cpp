#include "LienzoGrafo.h"
#include "ItemsGrafo.h"
#include "Tema.h"
#include "core/Recorrer.h"

#include <QContextMenuEvent>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QWheelEvent>
#include <climits>
#include <cmath>

using grafos::recorrer;

namespace {

const double MARGEN_ENTRE_NODOS = 14.0;  // espacio mínimo entre dos círculos
const double SEPARACION_PARALELAS = 34.0; // separación entre aristas del mismo par
const int NODOS_MAXIMOS_PARA_DESVIO = 400; // con más nodos se dibujan rectas (rendimiento)

QPointF unitario(QPointF v) {
    const double largo = std::hypot(v.x(), v.y());
    if (largo < 1e-9) return QPointF(1, 0);
    return v / largo;
}

double distancia(QPointF a, QPointF b) { return std::hypot(a.x() - b.x(), a.y() - b.y()); }

// Punto de una curva de Bézier cuadrática.
QPointF bezier(QPointF a, QPointF c, QPointF b, double t) {
    const double u = 1.0 - t;
    return a * (u * u) + c * (2 * u * t) + b * (t * t);
}

QPolygonF flecha(QPointF punta, QPointF desde) {
    const QPointF dir = unitario(punta - desde);
    const QPointF perp(-dir.y(), dir.x());
    const QPointF base = punta - dir * 11.0;
    QPolygonF p;
    p << punta << base + perp * 5.5 << base - perp * 5.5;
    return p;
}

QString formatoNumero(double v) { return QString::number(v, 'g', 6); }

// Baldosa de 26x26 con un punto: repetida, forma una grilla sutil (sin bucles).
QPixmap fondoPunteado() {
    QPixmap baldosa(26, 26);
    baldosa.fill(tema::panel());
    QPainter p(&baldosa);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(tema::linea());
    p.drawEllipse(QPointF(13, 13), 1.2, 1.2);
    return baldosa;
}

} // namespace

// ============================================================
//  Construcción
// ============================================================
LienzoGrafo::LienzoGrafo(grafos::Grafo* grafo, QWidget* parent)
    : QGraphicsView(parent), grafo_(grafo) {
    escena_ = new QGraphicsScene(this);
    setScene(escena_);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate); // repinta solo lo que cambió
    setBackgroundBrush(QBrush(fondoPunteado()));
    setCacheMode(QGraphicsView::CacheBackground);              // la grilla de puntos se dibuja una sola vez
    setMinimumSize(420, 300);

    temporizador_ = new QTimer(this);
    temporizador_->setInterval(500);
    connect(temporizador_, &QTimer::timeout, this, [this]() { avanzarAnimacion(); });

    reconstruir();
}

// ============================================================
//  Estado
// ============================================================
void LienzoGrafo::setFiltros(const grafos::Filtros& filtros) {
    filtros_ = filtros;
    aplicarEstados();
}

void LienzoGrafo::setOrigenDestino(int origen, int destino) {
    origen_ = origen;
    destino_ = destino;
    aplicarEstados();
}

void LienzoGrafo::setResultado(const grafos::Resultado* resultado, bool animar) {
    temporizador_->stop();
    camino_.clear();
    aristasCamino_.clear();
    if (resultado != nullptr && resultado->ok) {
        camino_ = resultado->camino;
        aristasCamino_ = resultado->aristasCamino;
    }
    const int total = static_cast<int>(camino_.size());
    if (animar && total > 1) {
        pasosVisibles_ = 1;
        iniciarAnimacion();
    } else {
        pasosVisibles_ = total;
    }
    aplicarEstados();
}

void LienzoGrafo::repetirAnimacion() {
    if (camino_.size() < 2) return;
    temporizador_->stop();
    pasosVisibles_ = 1;
    aplicarEstados();
    iniciarAnimacion();
}

// Un paso cada 0,5 s, pero un camino largo no tarda más de unos 6 segundos en mostrarse entero.
void LienzoGrafo::iniciarAnimacion() {
    const int pasos = static_cast<int>(camino_.size()) - 1;
    const int espera = pasos <= 12 ? 500 : (6000 / pasos < 100 ? 100 : 6000 / pasos);
    temporizador_->setInterval(espera);
    temporizador_->start();
}

void LienzoGrafo::avanzarAnimacion() {
    pasosVisibles_++;
    aplicarEstados();
    if (pasosVisibles_ >= static_cast<int>(camino_.size())) temporizador_->stop();
}

// ============================================================
//  Creación de los elementos
// ============================================================
void LienzoGrafo::reconstruir() {
    escena_->clear(); // borra todos los items
    nodos_.clear();
    aristas_.clear();
    etiquetas_.clear();

    const int n = grafo_->cantidadNodos();
    const int m = grafo_->cantidadAristas();

    if (n == 0) {
        auto* aviso = escena_->addSimpleText(
            QStringLiteral("Hacé clic acá para agregar un nodo, o cargá el ejemplo."));
        QFont f("Segoe UI");
        f.setPixelSize(15);
        aviso->setFont(f);
        aviso->setBrush(tema::apagado());
        aviso->setPos(260, 240);
    }

    // Nodos
    recorrer(0, n, [&](int j) {
        const grafos::Nodo& datos = grafo_->nodo(j);
        const QString nombre = QString::fromStdString(datos.nombre);
        auto* item = new NodoItem(j, nombre, radioParaNombre(nombre));
        item->setPos(datos.x, datos.y);
        item->habilitarArrastre();
        item->alMoverse = [this](int i, QPointF p) { nodoMoviendose(i, p); };
        item->alSoltarse = [this](int i) { nodoSoltado(i); };
        escena_->addItem(item);
        nodos_.push_back(item);
    });

    // Aristas y sus etiquetas
    recorrer(0, m, [&](int k) {
        const grafos::Arista& e = grafo_->arista(k);
        QString texto = formatoNumero(e.pesoIda);
        if (!e.dirigida && e.pesoVuelta != e.pesoIda)
            texto += QStringLiteral(" ⇄ ") + formatoNumero(e.pesoVuelta); // ⇄
        const QString a = QString::fromStdString(grafo_->nodo(e.desde).nombre);
        const QString b = QString::fromStdString(grafo_->nodo(e.hasta).nombre);

        auto* arista = new AristaItem(k);
        escena_->addItem(arista);
        aristas_.push_back(arista);

        auto* etiqueta = new EtiquetaItem(k, texto);
        if (e.dirigida)
            etiqueta->setToolTip(QStringLiteral("%1 → %2: %3").arg(a, b, formatoNumero(e.pesoIda)));
        else
            etiqueta->setToolTip(QStringLiteral("%1 → %2: %3\n%2 → %1: %4")
                                     .arg(a, b, formatoNumero(e.pesoIda), formatoNumero(e.pesoVuelta)));
        escena_->addItem(etiqueta);
        etiquetas_.push_back(etiqueta);
    });

    rutearAristas();
    aplicarEstados();
    actualizarRectEscena();
}

// ============================================================
//  Colores y resaltado del camino
// ============================================================
void LienzoGrafo::aplicarEstados() {
    const int n = static_cast<int>(nodos_.size());
    const int m = static_cast<int>(aristas_.size());
    if (n != grafo_->cantidadNodos() || m != grafo_->cantidadAristas()) return;

    // Primer paso en el que aparece cada nodo en el camino
    std::vector<int> primerPaso(n, INT_MAX);
    recorrer(0, static_cast<int>(camino_.size()), [&](int i) {
        const int v = camino_[i];
        if (v >= 0 && v < n && i < primerPaso[v]) primerPaso[v] = i;
    });

    // Sentido en que se recorre cada arista (solo los pasos ya visibles)
    std::vector<int> sentido(m, 0);
    recorrer(0, static_cast<int>(aristasCamino_.size()), [&](int i) {
        const int k = aristasCamino_[i];
        if (k < 0 || k >= m || i + 1 >= pasosVisibles_) return;
        const int este = (grafo_->arista(k).desde == camino_[i]) ? 1 : -1;
        sentido[k] = (sentido[k] != 0 && sentido[k] != este) ? 2 : este; // 2 = se recorre en los dos sentidos
    });

    std::vector<bool> excluido(n, false);
    recorrer(0, n, [&](int j) {
        excluido[j] = grafos::nodoExcluido(*grafo_, j, filtros_);
        const grafos::Nodo& datos = grafo_->nodo(j);
        QString rol;
        if (j == origen_ && j == destino_) rol = QStringLiteral("origen y destino");
        else if (j == origen_) rol = QStringLiteral("origen");
        else if (j == destino_) rol = QStringLiteral("destino");
        nodos_[j]->setEstado(datos.bloqueado, datos.obligatorio, excluido[j] && !datos.bloqueado,
                             primerPaso[j] < pasosVisibles_, rol);
    });

    recorrer(0, m, [&](int k) {
        const grafos::Arista& e = grafo_->arista(k);
        // Fuera si toca un nodo excluido o si TODOS sus sentidos tienen un peso bloqueado
        const bool idaCortada = grafos::pesoBloqueado(filtros_, e.pesoIda);
        const bool vueltaCortada = e.dirigida || grafos::pesoBloqueado(filtros_, e.pesoVuelta);
        const bool fuera = excluido[e.desde] || excluido[e.hasta] || (idaCortada && vueltaCortada);
        aristas_[k]->setEstado(e.dirigida, e.bloqueada, fuera, sentido[k]);
        etiquetas_[k]->setEstado(e.bloqueada, fuera, sentido[k] != 0);
    });
}

// ============================================================
//  Ruteo de aristas: curvas que esquivan los nodos
// ============================================================
void LienzoGrafo::rutearAristas() {
    recorrer(0, static_cast<int>(aristas_.size()), [&](int k) { rutearArista(k); });
}

// Mientras se arrastra un nodo solo se reacomodan sus aristas; al soltarlo se rutean todas.
void LienzoGrafo::rutearAristasDe(int nodo) {
    recorrer(0, static_cast<int>(aristas_.size()), [&](int k) {
        const grafos::Arista& e = grafo_->arista(k);
        if (e.desde == nodo || e.hasta == nodo) rutearArista(k);
    });
}

// Cuántas aristas unen el mismo par de nodos y en qué lugar está k.
void LienzoGrafo::posicionEnGrupo(int k, int j, int& posicion, int& tamanio) const {
    if (j >= grafo_->cantidadAristas()) return;
    const grafos::Arista& a = grafo_->arista(k);
    const grafos::Arista& b = grafo_->arista(j);
    const bool mismoPar = (a.desde == b.desde && a.hasta == b.hasta) || (a.desde == b.hasta && a.hasta == b.desde);
    if (mismoPar) {
        tamanio++;
        if (j < k) posicion++;
    }
    posicionEnGrupo(k, j + 1, posicion, tamanio);
}

bool LienzoGrafo::puntoLejosDeNodos(QPointF p, int excluirA, int excluirB, int j) const {
    if (j >= static_cast<int>(nodos_.size())) return true;
    if (j != excluirA && j != excluirB && distancia(p, nodos_[j]->pos()) < nodos_[j]->radio() + 6.0)
        return false;
    return puntoLejosDeNodos(p, excluirA, excluirB, j + 1);
}

// Revisa 19 puntos de la curva (t = 0.05 ... 0.95).
bool LienzoGrafo::curvaLibre(QPointF a, QPointF control, QPointF b, int excluirA, int excluirB, int muestra) const {
    if (muestra > 19) return true;
    if (!puntoLejosDeNodos(bezier(a, control, b, muestra / 20.0), excluirA, excluirB, 0)) return false;
    return curvaLibre(a, control, b, excluirA, excluirB, muestra + 1);
}

// Prueba desvíos 0, +30, -30, +60, -60 ... hasta encontrar una curva libre.
double LienzoGrafo::buscarDesvio(QPointF a, QPointF b, QPointF normal, double base,
                                 int excluirA, int excluirB, int intento) const {
    if (intento > 12) return base; // no hay lugar: queda la curva original
    double delta = ((intento + 1) / 2) * 30.0 * ((intento % 2 == 1) ? 1.0 : -1.0);
    if (base < 0) delta = -delta; // las curvas laterales se abren hacia afuera primero
    const double candidato = base + delta;
    const QPointF control = (a + b) / 2.0 + normal * (candidato * 2.0);
    if (curvaLibre(a, control, b, excluirA, excluirB, 1)) return candidato;
    return buscarDesvio(a, b, normal, base, excluirA, excluirB, intento + 1);
}

void LienzoGrafo::rutearArista(int k) {
    const grafos::Arista& e = grafo_->arista(k);
    const QPointF a = nodos_[e.desde]->pos();
    const QPointF b = nodos_[e.hasta]->pos();
    const double ra = nodos_[e.desde]->radio();
    const double rb = nodos_[e.hasta]->radio();

    // Normal calculada siempre del nodo de menor índice al de mayor,
    // para que A->B y B->A queden de lados opuestos.
    const int menor = e.desde < e.hasta ? e.desde : e.hasta;
    const int mayor = e.desde < e.hasta ? e.hasta : e.desde;
    const QPointF d = unitario(nodos_[mayor]->pos() - nodos_[menor]->pos());
    const QPointF normal(-d.y(), d.x());

    int posicion = 0;
    int tamanio = 0;
    posicionEnGrupo(k, 0, posicion, tamanio);
    const double base = (posicion - (tamanio - 1) / 2.0) * SEPARACION_PARALELAS;

    double desvio = base;
    if (static_cast<int>(nodos_.size()) <= NODOS_MAXIMOS_PARA_DESVIO)
        desvio = buscarDesvio(a, b, normal, base, e.desde, e.hasta, 0);

    const QPointF control = (a + b) / 2.0 + normal * (desvio * 2.0);
    const QPointF inicio = a + unitario(control - a) * ra;
    const QPointF fin = b + unitario(control - b) * rb;

    QPainterPath curva(inicio);
    curva.quadTo(control, fin);

    aristas_[k]->setGeometria(curva, flecha(fin, control), flecha(inicio, control));
    etiquetas_[k]->setPos(bezier(a, control, b, 0.5));
}

// ============================================================
//  Lugar libre para un nodo (no se superponen)
// ============================================================
bool LienzoGrafo::lugarLibre(QPointF p, double radio, int ignorar, int j) const {
    if (j >= grafo_->cantidadNodos()) return true;
    if (j != ignorar) {
        const grafos::Nodo& otro = grafo_->nodo(j);
        // El radio ya está calculado en el item: evita medir la fuente por cada nodo en cada intento.
        const double radioOtro = (j < static_cast<int>(nodos_.size()))
                                     ? nodos_[j]->radio()
                                     : radioParaNombre(QString::fromStdString(otro.nombre));
        if (distancia(p, QPointF(otro.x, otro.y)) < radio + radioOtro + MARGEN_ENTRE_NODOS) return false;
    }
    return lugarLibre(p, radio, ignorar, j + 1);
}

double LienzoGrafo::xMaxima(int j, double acumulado) const {
    if (j >= grafo_->cantidadNodos()) return acumulado;
    const double x = grafo_->nodo(j).x;
    return xMaxima(j + 1, x > acumulado ? x : acumulado);
}

// Espiral tipo "girasol": cada intento se aleja un poco más del punto pedido.
QPointF LienzoGrafo::buscarEnEspiral(QPointF centro, double radio, int ignorar, int intento) const {
    if (intento > 800) return QPointF(xMaxima(0, 0.0) + 2 * radio + 80, centro.y());
    const double angulo = intento * 2.39996323; // ángulo áureo
    const double lejania = 10.0 + radio * 0.9 * std::sqrt(static_cast<double>(intento));
    QPointF candidato = centro + QPointF(std::cos(angulo), std::sin(angulo)) * lejania;
    if (candidato.x() < radio + 10) candidato.setX(radio + 10);
    if (candidato.y() < radio + 24) candidato.setY(radio + 24);
    if (lugarLibre(candidato, radio, ignorar, 0)) return candidato;
    return buscarEnEspiral(centro, radio, ignorar, intento + 1);
}

QPointF LienzoGrafo::posicionLibre(QPointF deseada, double radio, int ignorar) const {
    QPointF p = deseada;
    if (p.x() < radio + 10) p.setX(radio + 10);
    if (p.y() < radio + 24) p.setY(radio + 24);
    if (lugarLibre(p, radio, ignorar, 0)) return p;
    return buscarEnEspiral(p, radio, ignorar, 1);
}

QPointF LienzoGrafo::centroVisible() const {
    return mapToScene(viewport()->rect().center());
}

// ============================================================
//  Arrastre de nodos
// ============================================================
void LienzoGrafo::nodoMoviendose(int indice, QPointF posicion) {
    if (indice < 0 || indice >= grafo_->cantidadNodos()) return;
    if (arrastrePendiente_) { // primer movimiento: se guarda cómo estaba para poder deshacerlo
        arrastrePendiente_ = false;
        if (alEmpezarArrastre) alEmpezarArrastre();
    }
    grafo_->nodo(indice).x = posicion.x();
    grafo_->nodo(indice).y = posicion.y();
    if (static_cast<int>(aristas_.size()) == grafo_->cantidadAristas()) rutearAristasDe(indice);
}

void LienzoGrafo::nodoSoltado(int indice) {
    if (indice < 0 || indice >= static_cast<int>(nodos_.size())) return;
    NodoItem* item = nodos_[indice];
    const QPointF libre = posicionLibre(item->pos(), item->radio(), indice);
    if (distancia(libre, item->pos()) > 0.5) item->setPos(libre); // dispara nodoMoviendose
    if (static_cast<int>(aristas_.size()) == grafo_->cantidadAristas()) rutearAristas(); // ruteo completo al soltar
    actualizarRectEscena();
    asegurarVisible(item->pos()); // un nodo soltado fuera de la vista no se pierde
}

// ============================================================
//  Vista
// ============================================================
void LienzoGrafo::actualizarRectEscena() {
    const QRectF contenido = escena_->itemsBoundingRect().adjusted(-120, -120, 120, 120);
    escena_->setSceneRect(contenido.united(QRectF(0, 0, 900, 520)));
}

namespace {
const double ZOOM_MINIMO = 0.2;
const double ZOOM_MAXIMO = 5.0;
}

// Multiplica el zoom actual, sin salirse de [ZOOM_MINIMO, ZOOM_MAXIMO]
void LienzoGrafo::escalarVista(double factor) {
    const double actual = transform().m11();
    double nuevo = actual * factor;
    if (nuevo < ZOOM_MINIMO) nuevo = ZOOM_MINIMO;
    if (nuevo > ZOOM_MAXIMO) nuevo = ZOOM_MAXIMO;
    if (actual > 0.0) scale(nuevo / actual, nuevo / actual);
}

void LienzoGrafo::asegurarVisible(QPointF punto) {
    const QRectF visible = mapToScene(viewport()->rect()).boundingRect();
    if (!visible.contains(punto)) centerOn(punto);
}

void LienzoGrafo::ajustarVista() {
    resetTransform();
    if (nodos_.empty()) {
        centerOn(450, 260);
        return;
    }
    const QRectF contenido = escena_->itemsBoundingRect().adjusted(-30, -30, 30, 30);
    fitInView(contenido, Qt::KeepAspectRatio);
    if (transform().m11() > 1.25) { // no agrandar de más los grafos chicos
        resetTransform();
        scale(1.25, 1.25);
        centerOn(contenido.center());
    }
}

// ============================================================
//  Mouse
// ============================================================
void LienzoGrafo::mousePressEvent(QMouseEvent* evento) {
    const QGraphicsItem* debajo = itemAt(evento->position().toPoint());
    arrastrePendiente_ = evento->button() == Qt::LeftButton && debajo != nullptr && debajo->type() == NodoItem::Type;
    QGraphicsView::mousePressEvent(evento);
}

// Los nodos se crean con doble clic en el fondo: un clic suelto (por ejemplo al arrastrar la vista) no crea nada.
void LienzoGrafo::mouseDoubleClickEvent(QMouseEvent* evento) {
    const QGraphicsItem* debajo = itemAt(evento->position().toPoint());
    const bool vacio = debajo == nullptr || debajo->type() == QGraphicsSimpleTextItem::Type; // el aviso no cuenta
    QGraphicsView::mouseDoubleClickEvent(evento);
    if (vacio && evento->button() == Qt::LeftButton && alHacerClicEnVacio)
        alHacerClicEnVacio(mapToScene(evento->position().toPoint()));
}

void LienzoGrafo::wheelEvent(QWheelEvent* evento) {
    if (evento->modifiers() & Qt::ControlModifier) {
        escalarVista(evento->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15);
        evento->accept();
        return;
    }
    evento->ignore(); // sin Ctrl, la rueda desplaza la página completa

}

void LienzoGrafo::contextMenuEvent(QContextMenuEvent* evento) {
    if (evento->reason() == QContextMenuEvent::Keyboard) return; // la tecla "Menú" no apunta a ningún nodo
    QGraphicsItem* item = itemAt(evento->pos());
    if (item == nullptr) return;
    if (item->type() == NodoItem::Type && alPedirMenuNodo) {
        alPedirMenuNodo(static_cast<NodoItem*>(item)->indice(), evento->globalPos());
    } else if (item->type() == AristaItem::Type && alPedirMenuArista) {
        alPedirMenuArista(static_cast<AristaItem*>(item)->indice(), evento->globalPos());
    } else if (item->type() == EtiquetaItem::Type && alPedirMenuArista) {
        alPedirMenuArista(static_cast<EtiquetaItem*>(item)->indiceArista(), evento->globalPos());
    }
}
