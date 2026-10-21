#pragma once

// ============================================================
//  Recorrer.h
//  Reemplazo recursivo del "for": recorre los índices [i, fin)
//  llamando a f(i). Solo usa if + recursión (consigna de la
//  materia: nada de for / while / do-while).
// ============================================================

namespace grafos {

template <class Funcion>
void recorrer(int i, int fin, const Funcion& f) {
    if (i >= fin) return;   // caso base
    f(i);                   // trabajo sobre el índice actual
    recorrer(i + 1, fin, f); // paso recursivo
}

} // namespace grafos
