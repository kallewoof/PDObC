//
// PDSplayTree.c
//
// Based on the Wikipedia article on splay trees.
//

#include <stdio.h>

#include "PDSplayTree.h"
#include "pd_internal.h"

#define right brc[1]
#define left  brc[0]
#define st_node_branch_for_key(node, k) node->brc[k > node->key]

void st_node_null(void *v) {}

typedef struct st_node *st_node;

struct PDSplayTree {
    PDDeallocator deallocator;
    PDInitiator   initiator;
    st_node       root;
    PDInteger     count;
};

struct st_node {
    PDInteger key;
    void *value;
    st_node parent;
    st_node brc[2];
};

st_node st_node_new(PDInteger key, void *value)
{
    st_node n = malloc(sizeof(struct st_node));
    n->key = key;
    n->value = value;
    n->brc[0] = n->brc[1] = n->parent = NULL;
    return n;
}

void st_node_destroy(st_node n, PDDeallocator deallocator)
{
    if (n->brc[0]) st_node_destroy(n->brc[0], deallocator);
    if (n->brc[1]) st_node_destroy(n->brc[1], deallocator);
    (*deallocator)(n->value);
    free(n);
}

void *st_node_find(st_node z, PDInteger key)
{
    while (z) {
        if (z->key == key) return z;
        z = st_node_branch_for_key(z, key);
    }
    
    return NULL;
}

st_node st_node_minimum(st_node u) 
{
    while (u->left) u = u->left;
    return u;
}

st_node st_node_maximum(st_node u)
{
    while (u->right) u = u->right;
    return u;
}

static inline PDInteger st_node_populate_keys(st_node root, PDInteger *dest)
{
    PDInteger i = 0;
    while (root) {
        if (root->brc[0]) i += st_node_populate_keys(root->brc[0], &dest[i]);
        if (root->value) {
            dest[i++] = root->key;
        }
        root = root->brc[1];
    }
    return i;
}

static inline PDBool st_node_iterate(st_node root, PDSplayTreeIterator it, void *userInfo)
{
    PDBool shouldStop = false;
    while (root) {
        if (root->brc[0]) 
            if (st_node_iterate(root->brc[0], it, userInfo)) return true;
        if (root->value) {
            it(root->key, root->value, userInfo, &shouldStop);
            if (shouldStop) return true;
        }
        root = root->brc[1];
    }
    return false;
}

//
//
//

// Private declarations

void PDSplayTreeRotateLeft(PDSplayTreeRef st, st_node x);
void PDSplayTreeRotateRight(PDSplayTreeRef st, st_node x);
void PDSplayTreeSplay(PDSplayTreeRef st, st_node x);
void PDSplayTreeReplace(PDSplayTreeRef st, st_node u, st_node v);

// Public implementations

void PDSplayTreeDestroy(PDSplayTreeRef st)
{
    if (st->root) st_node_destroy(st->root, st->deallocator);
}

PDSplayTreeRef PDSplayTreeCreateWithDeallocator(PDDeallocator deallocator)
{
    PDSplayTreeRef st = PDAllocTyped(PDInstanceTypeTree, sizeof(struct PDSplayTree), PDSplayTreeDestroy, false);
    st->deallocator = deallocator;
    st->initiator = NULL;
    st->count = 0;
    st->root = NULL;
    return st;
}

PDSplayTreeRef PDSplayTreeCreate(void)
{
    return PDSplayTreeCreateWithDeallocator(st_node_null);
}

void PDSplayTreeSetDeallocator(PDSplayTreeRef tree, PDDeallocator deallocator)
{
    tree->deallocator = deallocator;
}

void PDSplayTreeSetInitiator(PDSplayTreeRef tree, void *initiator)
{
    tree->initiator = initiator;
}

void PDSplayTreeInsert(PDSplayTreeRef tree, PDInteger key, void *value)
{
    if (tree->initiator) value = tree->initiator(value);
    
    st_node z = st_node_find(tree->root, key);
    if (z) {
        tree->deallocator(z->value);
        z->value = value;
        return;
    }
    z = tree->root;
    st_node p = NULL;
    
    while (z) {
        p = z;
        z = st_node_branch_for_key(z, key);
    }

    z = st_node_new(key, value);
    z->parent = p;
    
    if (!p)
        tree->root = z;
    else 
        st_node_branch_for_key(p, key) = z;

    PDSplayTreeSplay(tree, z);
    tree->count++;
}

void *PDSplayTreeGet(PDSplayTreeRef tree, PDInteger key)
{
    st_node z = st_node_find(tree->root, key);
    return z ? z->value : NULL;
}

void PDSplayTreeDelete(PDSplayTreeRef tree, PDInteger key)
{
    st_node z = st_node_find(tree->root, key);
    if (! z) return;

    PDSplayTreeSplay(tree, z);
    
    if (! z->left) 
        PDSplayTreeReplace(tree, z, z->right);
    else if (! z->right) 
        PDSplayTreeReplace(tree, z, z->left);
    else {
        st_node y = st_node_minimum(z->right);
        if (y->parent != z) {
            PDSplayTreeReplace(tree, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        PDSplayTreeReplace(tree, z, y);
        y->left = z->left;
        y->left->parent = y;
    }
    
    z->brc[0] = z->brc[1] = NULL; // necessary?
    st_node_destroy(z, tree->deallocator);
    tree->count--;
}

PDInteger PDSplayTreeGetCount(PDSplayTreeRef tree)
{
    return tree->count;
}

PDInteger PDSplayTreePopulateKeys(PDSplayTreeRef tree, PDInteger *dest)
{
    return st_node_populate_keys(tree->root, dest);
}

void PDSplayTreeIterate(PDSplayTreeRef tree, PDSplayTreeIterator it, void *userInfo)
{
    st_node_iterate(tree->root, it, userInfo);
}

// Private implementations

void PDSplayTreeRotateLeft(PDSplayTreeRef st, st_node x)
{
    st_node y = x->right;
    x->right = y->left;
    if (y->left) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (! x->parent) 
        st->root = y;
    else if (x == x->parent->left) 
        x->parent->left = y;
    else 
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

void PDSplayTreeRotateRight(PDSplayTreeRef st, st_node x)
{
    st_node y = x->left;
    x->left = y->right;
    if (y->right) y->right->parent = x;
    y->parent = x->parent;
    if (! x->parent) 
        st->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->right = x;
    x->parent = y;
}

void PDSplayTreeSplay(PDSplayTreeRef st, st_node x)
{
    while (x->parent) {
        if (! x->parent->parent) {
            if (x->parent->left == x)
                PDSplayTreeRotateRight(st, x->parent);
            else
                PDSplayTreeRotateLeft(st, x->parent);
        } else if (x->parent->left == x && x->parent->parent->left == x->parent) {
            PDSplayTreeRotateRight(st, x->parent->parent);
            PDSplayTreeRotateRight(st, x->parent);
        } else if (x->parent->right == x && x->parent->parent->right == x->parent) {
            PDSplayTreeRotateLeft(st, x->parent->parent);
            PDSplayTreeRotateLeft(st, x->parent);
        } else if (x->parent->left == x && x->parent->parent->right == x->parent) {
            PDSplayTreeRotateRight(st, x->parent);
            PDSplayTreeRotateLeft(st, x->parent);
        } else {
            PDSplayTreeRotateLeft(st, x->parent);
            PDSplayTreeRotateRight(st, x->parent);
        }
    }
}

void PDSplayTreeReplace(PDSplayTreeRef st, st_node u, st_node v)
{
    if (!u->parent)
        st->root = v;
    else if (u == u->parent->left) 
        u->parent->left = v;
    else
        u->parent->right = v;
    if (v) 
        v->parent = u->parent;
}

char snpb[20];
int st_node_print(st_node n, char **map, int x, int y)
{
    /*
   .-------------- (x,y)
   +---------
   |  (2)
   |  / \
   |(1)  (3)
   |
     */
    // because (x,y) is top left corner of node and all its left children printed out, we need to print left first
    if (n->brc[0]) {
        x = st_node_print(n->brc[0], map, x, y + 2);
        map[y+1][x] = '/';
    }
    sprintf(snpb, "(%ld)", n->key);
    for (int i = 0; snpb[i]; i++) {
        map[y][x++] = snpb[i];
    }
    if (n->brc[1]) {
        map[y+1][x-1] = '\\';
        x = st_node_print(n->brc[1], map, x, y+2);
    }
    return x;
}

void st_node_make_size(st_node n, int *w, int *h)
{
    int W = 3, H = 1;
    PDInteger z = n->key;
    while (z > 9) {
        W++;
        z /= 10;
    }
    
    int lw, rw, lh, rh;
    lw=rw=lh=rh=0;
    if (n->brc[0]) {
        // expand leftward
        //      (key)
        //      /
        //   (left)
        st_node_make_size(n->brc[0], &lw, &lh);
        W += lw;
    }
    if (n->brc[1]) {
        // expand rightward
        st_node_make_size(n->brc[1], &rw, &rh);
        W += rw;
    }
    if (rh || lh) {
        H += 1 + (rh > lh ? rh : lh);
    }
    *w = W;
    *h = H;
}

void PDSplayTreeShow(PDSplayTreeRef tree)
{
    int w, h;
    st_node_make_size(tree->root, &w, &h);
    char **map = malloc(sizeof(char*) * h);
    for (int i = 0; i < h; i++) {
        map[i] = malloc(w + 1);
        map[i][w] = 0;
        for (int j = 0; j < w; j++) {
            map[i][j] = ' ';
        }
    }
    st_node_print(tree->root, map, 0, 0);
    for (int i = 0; i < h; i++) {
        puts(map[i]);
    }
    free(map);
}
