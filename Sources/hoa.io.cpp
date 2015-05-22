/*
 // Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../hoa.library.h"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct _hoa_out
{
    t_eobj  f_obj;
    t_outlet *f_outlet;
    int    f_extra;
} t_hoa_out;

static t_eclass *hoa_out_class;

typedef struct _hoa_in
{
    t_eobj  f_obj;
    int    f_extra;
} t_hoa_in;

static t_eclass *hoa_in_class;

typedef struct _hoa_out_tilde
{
    t_edspobj   f_obj;
    t_sample*   f_signal;
    int         f_extra;
} t_hoa_out_tilde;

static t_eclass *hoa_out_tilde_class;

typedef struct _hoa_in_tilde
{
    t_edspobj   f_obj;
    t_sample*   f_signal;
    int         f_extra;
} t_hoa_in_tilde;

static t_eclass *hoa_intilde_class;

typedef struct _hoa_thisprocess
{
    t_eobj      j_box;
    char        f_nit;
    
    t_outlet*   f_out_hoa_args;
    t_outlet*   f_out_hoa_mode;
    t_outlet*   f_out_args;
    t_outlet*   f_out_attrs;
    t_outlet*   f_out_mute;
    
    t_atom      f_hoa_args[3];
    t_atom      f_hoa_mode[2];
    
    t_atom*     f_args;
    long        f_argc;
    
    long        f_n_attrs;
    t_symbol**  f_attr_name;
    t_atom*     f_attr_vals[EPD_MAX_SIGS];
    long        f_attr_size[EPD_MAX_SIGS];
    double      f_time;
} t_hoa_thisprocess;

static t_eclass *hoa_thisprocess_class;

static void *hoa_out_new(t_symbol *s, long argc, t_atom *argv)
{
    t_hoa_out *x = NULL;
    
    x = (t_hoa_out *)eobj_new(hoa_out_class);
    if(x)
    {
        inlet_new(&x->f_obj.o_obj, &x->f_obj.o_obj.ob_pd, 0, 0);
        x->f_outlet = NULL;
        x->f_extra = 0;
        if(argc > 1 && argv && atom_gettype(argv) == A_SYM && atom_gettype(argv+1) == A_FLOAT && atom_getsym(argv) == gensym("extra") && atom_getfloat(argv+1) > 0)
        {
            x->f_extra = atom_getfloat(argv+1);
        }
        else if(argc && argv)
        {
            pd_error(x, "wrong argument.");
            eobj_free(x);
            return NULL;
        }
    }
    
    return x;
}

static void hoa_out_bang(t_hoa_out *x)
{
    if(x->f_outlet)
        outlet_bang(x->f_outlet);
}

static void hoa_out_float(t_hoa_out *x, float f)
{
    if(x->f_outlet)
        outlet_float(x->f_outlet, f);
}

static void hoa_out_symbol(t_hoa_out *x, t_symbol* s)
{
    if(x->f_outlet)
        outlet_symbol(x->f_outlet, s);
}

static void hoa_out_list(t_hoa_out *x, t_symbol* s, int argc, t_atom* argv)
{
    if(x->f_outlet)
        outlet_list(x->f_outlet, s, argc, argv);
}

static void hoa_out_anything(t_hoa_out *x, t_symbol* s, int argc, t_atom* argv)
{
    if(x->f_outlet)
        outlet_anything(x->f_outlet, s, argc, argv);
}

extern "C" void setup_hoa0x2eout(void)
{
    t_eclass* c;
    c = eclass_new("hoa.out", (method)hoa_out_new, (method)eobj_free, (short)sizeof(t_hoa_out), CLASS_NOINLET, A_GIMME, 0);
    
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_out_bang,       "bang",     A_CANT,  0);
    eclass_addmethod(c, (method)hoa_out_float,      "float",    A_FLOAT, 0);
    eclass_addmethod(c, (method)hoa_out_symbol,     "symbol",   A_SYMBOL,0);
    eclass_addmethod(c, (method)hoa_out_list,       "list",     A_GIMME, 0);
    eclass_addmethod(c, (method)hoa_out_anything,   "anything", A_GIMME, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_out_class = c;
}

static void *hoa_in_new(t_symbol *s, long argc, t_atom *argv)
{
    t_hoa_in *x = NULL;
    
    x = (t_hoa_in *)eobj_new(hoa_in_class);
    if(x)
    {
        outlet_new(&x->f_obj.o_obj, 0);
        x->f_extra = 0;
        if(argc > 1 && argv && atom_gettype(argv) == A_SYM && atom_gettype(argv+1) == A_FLOAT && atom_getsym(argv) == gensym("extra") && atom_getfloat(argv+1) > 0)
        {
            x->f_extra = atom_getfloat(argv+1);
        }
        else if(argc && argv)
        {
            pd_error(x, "wrong argument.");
            eobj_free(x);
            return NULL;
        }
    }
    
    return x;
}

static void hoa_in_free(t_hoa_in *x)
{
    eobj_free(x);
}

static void hoa_in_bang(t_hoa_in *x)
{
    outlet_bang(x->f_obj.o_obj.ob_outlet);
}

static void hoa_in_float(t_hoa_in *x, float f)
{
    outlet_float(x->f_obj.o_obj.ob_outlet, f);
}

static void hoa_in_symbol(t_hoa_in *x, t_symbol* s)
{
    outlet_symbol(x->f_obj.o_obj.ob_outlet, s);
}

static void hoa_in_list(t_hoa_in *x, t_symbol* s, int argc, t_atom* argv)
{
    outlet_list(x->f_obj.o_obj.ob_outlet, s, argc, argv);
}

static void hoa_in_anything(t_hoa_in *x, t_symbol* s, int argc, t_atom* argv)
{
    outlet_anything(x->f_obj.o_obj.ob_outlet, s, argc, argv);
}

extern "C" void setup_hoa0x2ein(void)
{
    t_eclass* c;
    c = eclass_new("hoa.in", (method)hoa_in_new, (method)hoa_in_free, (short)sizeof(t_hoa_in), CLASS_NOINLET, A_GIMME, 0);
    
    hoa_initclass(c);
    class_sethelpsymbol((t_class *)c, gensym("help/hoa.io"));
    eclass_addmethod(c, (method)hoa_in_bang,       "bang",     A_CANT,  0);
    eclass_addmethod(c, (method)hoa_in_float,      "float",    A_FLOAT, 0);
    eclass_addmethod(c, (method)hoa_in_symbol,     "symbol",   A_SYMBOL,0);
    eclass_addmethod(c, (method)hoa_in_list,       "list",     A_GIMME, 0);
    eclass_addmethod(c, (method)hoa_in_anything,   "anything", A_GIMME, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_in_class = c;
}

static void *hoa_out_tilde_new(t_symbol *s, long argc, t_atom *argv)
{
    t_hoa_out_tilde *x = NULL;
    
    x = (t_hoa_out_tilde *)eobj_new(hoa_out_tilde_class);
	if(x)
	{
        eobj_dspsetup(x, 0, 0);
        x->f_extra = 0;
        x->f_signal = NULL;
        if(argc > 1 && argv && atom_gettype(argv) == A_SYM && atom_gettype(argv+1) == A_FLOAT && atom_getsym(argv) == gensym("extra") && atom_getfloat(argv+1) > 0)
        {
            x->f_extra = atom_getfloat(argv+1);
        }
        else if(argc && argv)
        {
            pd_error(x, "wrong argument.");
            eobj_free(x);
            return NULL;
        }
	}
    
	return x;
}

static void hoa_out_tilde_perform(t_hoa_out_tilde *x, t_object *dsp, float **inps, long ni, float **outs, long no, long sf, long f,void *up)
{
    cblas_saxpy(sf, 1.f, inps[0], 1, x->f_signal, 1);
}

static void hoa_out_tilde_dsp(t_hoa_out_tilde *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    if(x->f_signal)
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_out_tilde_perform, 0, NULL);
}

extern "C" void setup_hoa0x2eout_tilde(void)
{
    t_eclass* c;
    c = eclass_new("hoa.out~", (method)hoa_out_tilde_new, (method)eobj_dspfree, (short)sizeof(t_hoa_out_tilde), 0, A_GIMME, 0);
    
    hoa_initclass(c);
    eclass_dspinit(c);
    eclass_addmethod(c, (method)hoa_out_tilde_dsp, "dsp", A_CANT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_out_tilde_class = c;
    
}

static void *hoa_intilde_new(t_symbol *s, long argc, t_atom *argv)
{
    t_hoa_in_tilde *x = NULL;
    
    x = (t_hoa_in_tilde *)eobj_new(hoa_intilde_class);
    if(x)
    {
        eobj_dspsetup(x, 0, 1);
        x->f_extra = 0;
        x->f_signal = NULL;
        if(argc > 1 && argv && atom_gettype(argv) == A_SYM && atom_gettype(argv+1) == A_FLOAT && atom_getsym(argv) == gensym("extra") && atom_getfloat(argv+1) > 0)
        {
            x->f_extra = atom_getfloat(argv+1);
        }
        else if(argc && argv)
        {
            pd_error(x, "wrong argument.");
            eobj_free(x);
            return NULL;
        }
    }
    
    return x;
}

static void hoa_intilde_perform(t_hoa_in_tilde *x, t_object *dsp, float **ins, long ni, float **outs, long no, long sf, long f,void *up)
{
    memcpy(outs[0], x->f_signal, sf * sizeof(t_sample));
}

static void hoa_intilde_perform_zero(t_hoa_in_tilde *x, t_object *dsp, float **ins, long ni, float **outs, long no, long sf, long f,void *up)
{
    memset(outs[0], 0, sf * sizeof(t_sample));
}


static void hoa_intilde_dsp(t_hoa_in_tilde *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    if(x->f_signal)
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_intilde_perform, 0, NULL);
    else
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_intilde_perform_zero, 0, NULL);
}

extern "C" void setup_hoa0x2ein_tilde(void)
{
    t_eclass* c;
    c = eclass_new("hoa.in~", (method)hoa_intilde_new, (method)eobj_dspfree, (short)sizeof(t_hoa_in_tilde), CLASS_NOINLET, A_GIMME, 0);
    
    hoa_initclass(c);
    eclass_dspinit(c);
    eclass_addmethod(c, (method)hoa_intilde_dsp, "dsp", A_CANT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_intilde_class = c;
    
}

static void *hoa_thisprocess_new(t_symbol *s, long argc, t_atom *argv)
{
    int i;
    t_hoa_thisprocess *x =  NULL;
    
    x = (t_hoa_thisprocess *)eobj_new(hoa_thisprocess_class);
    if(x)
    {
        x->f_nit = 0;
        x->f_argc = atoms_get_attributes_offset(argc, argv);
        x->f_args = (t_atom *)calloc(x->f_argc, sizeof(t_atom));
        for(i = 0; i < x->f_argc; i++)
        {
            x->f_args[i] = argv[i];
        }

        x->f_n_attrs = atoms_get_keys(argc-x->f_argc, argv+x->f_argc, &x->f_attr_name);
        for(i = 0; i < x->f_n_attrs; i++)
        {
            atoms_get_attribute(argc-x->f_argc, argv+x->f_argc, x->f_attr_name[i], &x->f_attr_size[i], &x->f_attr_vals[i]);
        }
        
        x->f_out_hoa_args = listout(x);
        x->f_out_hoa_mode = anythingout(x);
        x->f_out_args     = anythingout(x);
        x->f_out_attrs    = anythingout(x);
        
        x->f_time = clock_getsystime();
    }
    
    return (x);
}

static void hoa_thisprocess_bang(t_hoa_thisprocess *x)
{
    char        attr_char[MAXPDSTRING];
    for(int i = 0; i < x->f_n_attrs; i++)
    {
        sprintf(attr_char, "%s", x->f_attr_name[i]->s_name+1);
        outlet_anything(x->f_out_attrs, gensym(attr_char), x->f_attr_size[i], x->f_attr_vals[i]);
    }
    
    if(x->f_argc && x->f_args)
        outlet_list(x->f_out_args, &s_list, x->f_argc, x->f_args);
    if(x->f_nit)
    {
        outlet_list(x->f_out_hoa_mode,  &s_list, 2, x->f_hoa_mode);
        if(atom_getsym(x->f_hoa_mode) == gensym("2d"))
            outlet_list(x->f_out_hoa_args, &s_list, 2, x->f_hoa_args);
        else if(atom_getsym(x->f_hoa_mode) == gensym("3d") && atom_getsym(x->f_hoa_mode+1) == gensym("planewaves"))
            outlet_list(x->f_out_hoa_args, &s_list, 2, x->f_hoa_args);
        else
            outlet_list(x->f_out_hoa_args, &s_list, 3, x->f_hoa_args);
    }
    outlet_symbol(x->f_out_attrs, gensym("done"));
}

static void hoa_thisprocess_click(t_hoa_thisprocess *x)
{
    if(clock_gettimesince(x->f_time) < 250.)
        hoa_thisprocess_bang(x);
    x->f_time = clock_getsystime();
}

static void hoa_thisprocess_free(t_hoa_thisprocess *x)
{
    int i;
    if(x->f_argc && x->f_args)
    {
        free(x->f_args);
    }
    if(x->f_n_attrs)
    {
        for(i = 0; i < x->f_n_attrs; i++)
        {
            if(x->f_attr_size[i])
                free(x->f_attr_vals[i]);
        }
    }
    free(x->f_attr_name);
    eobj_free(x);
}

extern "C" void setup_hoa0x2ethisprocess_tilde(void)
{
    t_eclass* c;
    c = eclass_new("hoa.thisprocess~", (method)hoa_thisprocess_new, (method)hoa_thisprocess_free, (short)sizeof(t_hoa_thisprocess), 0, A_GIMME, 0);
    
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_thisprocess_bang,       "bang",     A_CANT, 0);
    eclass_addmethod(c, (method)hoa_thisprocess_click,      "click",    A_CANT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_thisprocess_class = c;
}


