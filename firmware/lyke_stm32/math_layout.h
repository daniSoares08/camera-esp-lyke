#pragma once

// Display-only syntax tree. Calculation continues to use scientific.h unchanged.
// Coordinates are the 126 x 48 visible viewport, before the LCD's 180-degree map.
enum { ML_TEXT, ML_LITERAL, ML_BINARY, ML_FRACTION, ML_POWER, ML_ROOT,
       ML_GROUP, ML_FUNCTION, ML_PREFIX, ML_POSTFIX, ML_MIXED, ML_EMPTY };
typedef struct {
    uint16_t w, up, down;
    uint8_t kind, left, right, start, end, token;
} MathNode;
typedef struct {
    MathNode nodes[200];
    const uint8_t *input;
    unsigned count, at, depth, cursor;
    int cx, cy, shift;
    bool failed, painting;
    void (*pixel)(int, int);
} MathLayout;
static MathLayout math_layout;

static const char *math_literal(unsigned c)
{
    if (c==POW10) return "10";
    if (c==EXP) return "e";
    return c==SQUARE ? "2" : c==CUBE ? "3" : "-1";
}

static unsigned math_node(unsigned kind, unsigned left, unsigned right, unsigned start, unsigned end, unsigned token)
{
    MathLayout *m=&math_layout;
    if (m->count>=200) {m->failed=true; return 0;}
    unsigned index=m->count++;
    MathNode *n=&m->nodes[index], *a=&m->nodes[left], *b=&m->nodes[right];
    *n=(MathNode){.kind=kind,.left=left,.right=right,.start=start,.end=end,.token=token,.up=3,.down=3};
    if (kind==ML_TEXT) n->w=token?strlen(token_text(token))*6:(end-start)*6;
    else if (kind==ML_LITERAL) n->w=strlen(math_literal(token))*6;
    else if (kind==ML_EMPTY) n->w=6;
    else {
        n->up=a->up; n->down=a->down; n->w=a->w;
        if (kind==ML_FRACTION) {
            n->w=(a->w>b->w?a->w:b->w)+4;
            n->up=a->up+a->down+2; n->down=b->up+b->down+2;
        } else if (kind==ML_POWER) {
            n->w=a->w+b->w+1; n->up=a->up+1+b->up+b->down+1;
        } else if (kind==ML_ROOT) {
            n->w=a->w+11+(token?b->w:0); n->up=a->up+3; n->down=a->down+1;
            if (token) n->up+=b->up+b->down;
        } else if (kind==ML_GROUP) n->w+=8;
        else if (kind==ML_FUNCTION) n->w+=8+(strlen(token_text(token))-1)*6;
        else if (kind==ML_PREFIX || kind==ML_POSTFIX) n->w+=6;
        else {
            n->w+=b->w+(kind==ML_MIXED?2:7);
            if (b->up>n->up) n->up=b->up;
            if (b->down>n->down) n->down=b->down;
        }
    }
    return index;
}

static unsigned math_sum(void);
static unsigned math_sign(void);
static unsigned math_atom(void)
{
    MathLayout *m=&math_layout;
    unsigned start=m->at, c=m->input[start];
    if (!c || c==')' || c==',') return math_node(ML_EMPTY,0,0,start,start,0);
    ++m->at;
    if ((c>='0'&&c<='9') || c=='.') {
        while ((m->input[m->at]>='0' && m->input[m->at]<='9') || m->input[m->at]=='.' ||
               m->input[m->at]=='E' || ((m->input[m->at]=='+' || m->input[m->at]=='-') &&
               m->input[m->at-1]=='E')) ++m->at;
        return math_node(ML_TEXT,0,0,start,m->at,0);
    }
    if (c=='(' || (c>=SQRT && c<=REC) || c==RND) {
        if (++m->depth>24) {m->failed=true; --m->depth; return 0;}
        unsigned child=math_sum();
        if (m->input[m->at]==',') {
            ++m->at; unsigned second=math_sum();
            child=math_node(ML_BINARY,child,second,start+1,m->at,',');
        }
        if (m->input[m->at]==')') ++m->at;
        --m->depth;
        if (c==SQRT || c==CBRT) {
            unsigned index=c==CBRT?math_node(ML_LITERAL,0,0,start,start+1,CUBE):0;
            return math_node(ML_ROOT,child,index,start,m->at,c==CBRT);
        }
        if (c==POW10 || c==EXP) {
            unsigned base=math_node(ML_LITERAL,0,0,start,start+1,c);
            return math_node(ML_POWER,base,child,start,m->at,0);
        }
        return math_node(c=='('?ML_GROUP:ML_FUNCTION,child,0,start,m->at,c);
    }
    if (c>=PI && c<=GREEK_OMEGA)
        return math_node(ML_TEXT,0,0,start,m->at,c);
    m->failed=true;
    return math_node(ML_TEXT,0,0,start,m->at,0);
}

static unsigned math_postfix(void)
{
    MathLayout *m=&math_layout; unsigned value=math_atom();
    for (;;) {
        unsigned c=m->input[m->at], at=m->at;
        if (c==SQUARE || c==CUBE || c==RECIP) {
            ++m->at; unsigned exponent=math_node(ML_LITERAL,0,0,at,m->at,c);
            value=math_node(ML_POWER,value,exponent,m->nodes[value].start,m->at,0);
        } else if (c==FACT || c==PCT) {
            ++m->at; value=math_node(ML_POSTFIX,value,0,m->nodes[value].start,m->at,c==FACT?'!':'%');
        } else return value;
    }
}

static unsigned math_sign(void)
{
    MathLayout *m=&math_layout;
    if (++m->depth>24) {m->failed=true;--m->depth;return 0;}
    unsigned start=m->at, c=m->input[start], value;
    if (c=='-' || c=='+') {
        ++m->at; value=math_sign();value=math_node(ML_PREFIX,value,0,start,m->at,c);
    } else {
        value=math_postfix();c=m->input[m->at];
        if (c=='^' || c==ROOT) {
            ++m->at; unsigned b=math_sign();
            value=c=='^'?math_node(ML_POWER,value,b,start,m->at,0):math_node(ML_ROOT,b,value,start,m->at,1);
        }
    }
    --m->depth;return value;
}

static unsigned math_term(void)
{
    MathLayout *m=&math_layout;unsigned a=math_sign();
    for (;;) {
        unsigned c=m->input[m->at], b;
        if (c=='*' || c=='/' || c==FRAC || c==NCR || c==NPR) {
            if (c==NCR || c==NPR) {m->failed=true;return a;}
            ++m->at;b=math_sign();
            if (c==FRAC && m->input[m->at]==FRAC) {
                ++m->at;unsigned denominator=math_sign();
                b=math_node(ML_FRACTION,b,denominator,m->nodes[b].start,m->at,0);
                a=math_node(ML_MIXED,a,b,m->nodes[a].start,m->at,0);
            } else a=math_node(c==FRAC?ML_FRACTION:ML_BINARY,a,b,m->nodes[a].start,m->at,
                               c=='*'?GLYPH_TIMES:c=='/'?GLYPH_DIVIDE:c);
        } else if (c=='(' || c==PI || c==EULER || c==ANS || (c>=SQRT && c<=GREEK_OMEGA)) {
            b=math_sign();a=math_node(ML_BINARY,a,b,m->nodes[a].start,m->at,GLYPH_TIMES);
        } else return a;
        if (m->failed) return a;
    }
}

static unsigned math_sum(void)
{
    MathLayout *m=&math_layout;unsigned a=math_term();
    while (m->input[m->at]=='+' || m->input[m->at]=='-') {
        unsigned c=m->input[m->at++], b=math_term();
        a=math_node(ML_BINARY,a,b,m->nodes[a].start,m->at,c);
        if (m->failed) break;
    }
    return a;
}

static void math_dot(int x, int y)
{
    MathLayout *m=&math_layout;
    if (m->painting && m->pixel && x>=m->shift && x<m->shift+126 && y>=8 && y<48)
        m->pixel(x-m->shift,y);
}

static void math_stroke(int x, int y, int xx, int yy)
{
    int dx=abs(xx-x), sx=x<xx?1:-1, dy=-abs(yy-y), sy=y<yy?1:-1, error=dx+dy;
    for (;;) {
        math_dot(x,y);if (x==xx && y==yy) break;
        int e=error*2;if (e>=dy) {error+=dy;x+=sx;} if (e<=dx) {error+=dx;y+=sy;}
    }
}

static void math_char(unsigned code, int x, int axis)
{
    const uint8_t *p=calc_glyph(code);
    for (unsigned col=0;col<5;++col) for(unsigned row=0;row<7;++row)
        if (p[col]&(1u<<row)) math_dot(x+col,axis-3+row);
}

static void math_mark(unsigned position,int x,int axis)
{
    if (math_layout.cursor==position) {math_layout.cx=x;math_layout.cy=axis+5;}
}

static void math_paint(unsigned id,int x,int axis)
{
    MathLayout *m=&math_layout;MathNode *n=&m->nodes[id], *a=&m->nodes[n->left], *b=&m->nodes[n->right];
    math_mark(n->start,x,axis);
    if (n->kind==ML_TEXT || n->kind==ML_LITERAL) {
        if (n->kind==ML_LITERAL || n->token) {
            const char *s=n->kind==ML_LITERAL?math_literal(n->token):token_text(n->token);
            while (*s) {math_char((uint8_t)*s++,x,axis);x+=6;}
        } else for (unsigned i=n->start;i<n->end;++i) {math_mark(i,x,axis);math_char(m->input[i],x,axis);x+=6;}
    } else if (n->kind==ML_EMPTY) {
        math_stroke(x,axis-2,x+4,axis-2);math_stroke(x,axis+2,x+4,axis+2);
        math_stroke(x,axis-2,x,axis+2);math_stroke(x+4,axis-2,x+4,axis+2);x+=6;
    } else if (n->kind==ML_FRACTION) {
        math_paint(n->left,x+(n->w-a->w)/2,axis-2-a->down);
        math_stroke(x,axis,x+n->w-1,axis);
        math_paint(n->right,x+(n->w-b->w)/2,axis+2+b->up);x+=n->w;
    } else if (n->kind==ML_POWER) {
        math_paint(n->left,x,axis);math_paint(n->right,x+a->w+1,axis-a->up-1-b->down);x+=n->w;
    } else if (n->kind==ML_ROOT) {
        int bar=axis-a->up-2, lead=n->token?b->w:0;
        if (n->token) math_paint(n->right,x,bar-b->down);
        math_stroke(x+lead,axis,x+lead+2,axis-1);
        math_stroke(x+lead+2,axis-1,x+lead+5,axis+a->down+1);
        math_stroke(x+lead+5,axis+a->down+1,x+lead+9,bar);
        math_stroke(x+lead+9,bar,x+n->w-1,bar);
        math_paint(n->left,x+lead+11,axis);x+=n->w;
    } else if (n->kind==ML_GROUP || n->kind==ML_FUNCTION) {
        if (n->kind==ML_FUNCTION) {
            const char *s=token_text(n->token);
            while (s[0] && s[1]) {math_char((uint8_t)*s++,x,axis);x+=6;}
        }
        int top=axis-a->up,bottom=axis+a->down;
        math_stroke(x+2,top,x,top+2);math_stroke(x,top+2,x,bottom-2);math_stroke(x,bottom-2,x+2,bottom);
        math_paint(n->left,x+4,axis);x+=a->w+5;
        math_stroke(x,top,x+2,top+2);math_stroke(x+2,top+2,x+2,bottom-2);math_stroke(x+2,bottom-2,x,bottom);x+=3;
    } else if (n->kind==ML_PREFIX) {
        math_char(n->token,x,axis);math_paint(n->left,x+6,axis);x+=n->w;
    } else {
        math_paint(n->left,x,axis);x+=a->w;
        if (n->kind==ML_POSTFIX) {math_char(n->token,x,axis);x+=6;}
        else {
            if (n->kind!=ML_MIXED) math_char(n->token,x,axis);
            x+=n->kind==ML_MIXED?2:7;math_paint(n->right,x,axis);x+=b->w;
        }
    }
    math_mark(n->end,x,axis);
}

// Returns used height, or zero for linear fallback (malformed/too tall).
static unsigned math_draw(const uint8_t *input,unsigned cursor,bool caret,int top,unsigned max_height,
                           void (*pixel)(int,int))
{
    MathLayout *m=&math_layout;
    memset(m,0,sizeof(*m));m->input=input;m->cursor=cursor;m->pixel=pixel;
    unsigned root=math_sum();MathNode *n=&m->nodes[root];
    unsigned height=n->up+n->down+1+(caret?2:0);
    if (m->failed || input[m->at] || height>max_height) return 0;
    m->cx=n->w;m->cy=top+n->up+5;
    math_paint(root,0,top+n->up);
    m->shift=m->cx>119?m->cx-119:0;m->painting=true;
    math_paint(root,0,top+n->up);
    if (caret) math_stroke(m->cx,m->cy,m->cx+4,m->cy);
    return height;
}
