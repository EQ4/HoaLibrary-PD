/*
// Copyright (c) 2012-2015 Eliott Paris, Julien Colafrancesco, Thomas Le Meur & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.library.hpp"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct _hoa_decoder
{
    t_edspobj                   f_obj;
    Decoder<Hoa2d, t_sample>*   f_decoder;
    t_sample*                   f_ins;
    t_sample*                   f_outs;
    t_symbol*                   f_mode;
    void*                       f_attrs;
} t_hoa_decoder;

static t_eclass *hoa_decoder_class;

typedef struct _hoa_decoder_3d
{
    t_edspobj                   f_obj;
    Decoder<Hoa3d, t_sample>*   f_decoder;
    t_sample*                   f_ins;
    t_sample*                   f_outs;
    t_symbol*                   f_mode;
    void*                       f_attrs;
} t_hoa_decoder_3d;

static t_eclass *hoa_decoder_3d_class;

static void *hoa_decoder_new(t_symbol *s, int argc, t_atom *argv)
{
    ulong order = 1;
    ulong arg   = 4;
    t_symbol* mode = gensym("regular");
    t_hoa_decoder *x = (t_hoa_decoder *)eobj_new(hoa_decoder_class);
    t_binbuf *d      = binbuf_via_atoms(argc,argv);

    if(x && d)
	{
        if(argc && argv && atom_gettype(argv) == A_LONG)
        {
            order = ulong(pd_clip_minmax(atom_getlong(argv), 1, 63));
        }
        if(argc > 1 && argv+1 && atom_gettype(argv+1) == A_SYM)
            mode = atom_getsym(argv+1);

        if(mode == gensym("irregular"))
        {
            arg = order * 2 + 1;
            if(argc > 2 && argv+2 && atom_gettype(argv+2) == A_LONG)
            {
                arg = ulong(pd_clip_min(atom_getlong(argv+2), 1));
            }
            x->f_decoder = new Decoder<Hoa2d, t_sample>::Irregular(order, arg);
            x->f_mode = mode;
        }
        else if(mode == gensym("binaural"))
        {
            x->f_decoder = new Decoder<Hoa2d, t_sample>::Binaural(order);
            x->f_mode = mode;
        }
        else
        {
            arg = order * 2 + 1;
            if(argc > 2 && argv+2 && atom_gettype(argv+2) == A_LONG)
            {
                arg = pd_clip_min(atom_getlong(argv+2), 1);
            }
            x->f_decoder = new Decoder<Hoa2d, t_sample>::Regular(order, arg);
            x->f_mode = gensym("regular");
        }

        eobj_dspsetup(x, long(x->f_decoder->getNumberOfHarmonics()), long(x->f_decoder->getNumberOfPlanewaves()));
        x->f_ins = Signal<t_sample>::alloc(x->f_decoder->getNumberOfHarmonics() * HOA_MAXBLKSIZE);
        x->f_outs= Signal<t_sample>::alloc(x->f_decoder->getNumberOfPlanewaves() * HOA_MAXBLKSIZE);
        ebox_attrprocess_viabinbuf(x, d);

        return x;
	}

    return NULL;
}

static void hoa_decoder_perform_regular(t_hoa_decoder *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_ins+i, ulong(numins));
    }
	for(long i = 0; i < sampleframes; i++)
    {
        (static_cast<Decoder<Hoa2d, t_sample>::Regular*>(x->f_decoder))->process(x->f_ins + numins * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_decoder_perform_irregular(t_hoa_decoder *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_ins+i, ulong(numins));
    }
    for(long i = 0; i < sampleframes; i++)
    {
       (static_cast<Decoder<Hoa2d, t_sample>::Irregular*>(x->f_decoder))->process(x->f_ins + numins * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_decoder_perform_binaural(t_hoa_decoder *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    (static_cast<Decoder<Hoa2d, t_sample>::Binaural*>(x->f_decoder))->processBlock((const t_sample**)ins, outs);
}

static void hoa_decoder_dsp(t_hoa_decoder *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    x->f_decoder->computeRendering(ulong(maxvectorsize));
    if(x->f_mode == gensym("binaural"))
    {
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_decoder_perform_binaural, 0, NULL);
    }
    else if(x->f_mode == gensym("irregular"))
    {
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_decoder_perform_irregular, 0, NULL);
    }
    else
    {
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_decoder_perform_regular, 0, NULL);
    }
}

static t_pd_err hoa_decoder_angles_set(t_hoa_decoder *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        int dspState = canvas_suspend_dsp();
        for(long i = 0; i < argc && i < (long)x->f_decoder->getNumberOfPlanewaves(); i++)
        {
            if(atom_gettype(argv+i) == A_FLOAT)
                x->f_decoder->setPlanewaveAzimuth(ulong(i), atom_getfloat(argv+i) / 360. * HOA_2PI);
        }
        canvas_resume_dsp(dspState);
    }

    return 0;
}

static t_pd_err hoa_decoder_angles_get(t_hoa_decoder *x, void *attr, int* argc, t_atom **argv)
{
    *argc = long(x->f_decoder->getNumberOfPlanewaves());
    *argv = (t_atom *)malloc(size_t(*argc) * sizeof(t_atom));
    if(*argc && *argv)
    {
        for(int i = 0; i < *argc; i++)
        {
             atom_setfloat(*argv+i, x->f_decoder->getPlanewaveAzimuth(ulong(i)) * 360. / HOA_2PI);
        }
    }
    else
    {
        *argc = 0;
        *argv = NULL;
    }
    return 0;
}

static t_pd_err hoa_decoder_offset_set(t_hoa_decoder *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv && atom_gettype(argv) == A_FLOAT)
    {
        int dspState = canvas_suspend_dsp();
        x->f_decoder->setPlanewavesRotation(0., 0., atom_getfloat(argv) / 360. * HOA_2PI);
        canvas_resume_dsp(dspState);
    }
    return 0;
}

static t_pd_err hoa_decoder_offset_get(t_hoa_decoder *x, void *attr, int* argc, t_atom **argv)
{
    *argc = 1;
    *argv = (t_atom *)malloc(size_t(*argc) * sizeof(t_atom));
    if(*argc && *argv)
    {
        atom_setfloat(*argv, x->f_decoder->getPlanewavesRotationZ() * 360. / HOA_2PI);
    }
    else
    {
        *argc = 0;
        *argv = NULL;
    }
    return 0;
}

static t_pd_err hoa_decoder_crop_set(t_hoa_decoder *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv && atom_gettype(argv) == A_FLOAT)
    {
        if(x->f_mode == gensym("binaural"))
        {
            int dspState = canvas_suspend_dsp();
            (static_cast<Decoder<Hoa2d, t_sample>::Binaural*>(x->f_decoder))->setCropSize(atom_getfloat(argv));
            canvas_resume_dsp(dspState);
        }
    }
    return 0;
}

static t_pd_err hoa_decoder_crop_get(t_hoa_decoder *x, void *attr, int* argc, t_atom **argv)
{
    *argc = 1;
    *argv = (t_atom *)malloc(size_t(*argc) * sizeof(t_atom));
    if(*argc && *argv)
    {
        if(x->f_mode == gensym("binaural"))
        {
            atom_setfloat(*argv, (static_cast<Decoder<Hoa2d, t_sample>::Binaural*>(x->f_decoder))->getCropSize());
        }
        else
        {
            atom_setfloat(*argv, 0);
        }
    }
    else
    {
        *argc = 0;
        *argv = NULL;
    }
    return 0;
}

static void hoa_decoder_free(t_hoa_decoder *x)
{
    eobj_dspfree(x);
	delete x->f_decoder;
    Signal<t_sample>::free(x->f_ins);
    Signal<t_sample>::free(x->f_outs);
}

extern "C" void setup_hoa0x2e2d0x2edecoder_tilde(void)
{
    t_eclass* c;

    c = eclass_new("hoa.2d.decoder~", (method)hoa_decoder_new, (method)hoa_decoder_free, (short)sizeof(t_hoa_decoder), 0L, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_decoder_new, gensym("hoa.decoder~"), A_GIMME, 0);
    eclass_dspinit(c);
    eclass_addmethod(c, (method)hoa_decoder_dsp,           "dsp",          A_CANT,  0);

    CLASS_ATTR_DOUBLE_VARSIZE	(c, "angles",0, t_hoa_decoder, f_attrs, f_attrs, HOA_MAX_PLANEWAVES);
    CLASS_ATTR_ACCESSORS		(c, "angles", hoa_decoder_angles_get, hoa_decoder_angles_set);
    CLASS_ATTR_CATEGORY			(c, "angles", 0, "Planewaves");
    CLASS_ATTR_LABEL			(c, "angles", 0, "Angles of Loudspeakers");
    CLASS_ATTR_SAVE             (c, "angles", 0);

    CLASS_ATTR_DOUBLE           (c, "offset", 0, t_hoa_decoder, f_attrs);
    CLASS_ATTR_ACCESSORS		(c, "offset", hoa_decoder_offset_get, hoa_decoder_offset_set);
    CLASS_ATTR_CATEGORY			(c, "offset", 0, "Planewaves");
    CLASS_ATTR_LABEL            (c, "offset", 0, "Offset of Channels");
    CLASS_ATTR_SAVE             (c, "offset", 0);
    
    CLASS_ATTR_LONG             (c, "crop", 0, t_hoa_decoder, f_attrs);
    CLASS_ATTR_ACCESSORS		(c, "crop", hoa_decoder_crop_get, hoa_decoder_crop_set);
    CLASS_ATTR_CATEGORY			(c, "crop", 0, "Planewaves");
    CLASS_ATTR_LABEL            (c, "crop", 0, "Crop of the Responses");
    CLASS_ATTR_SAVE             (c, "crop", 0);

    eclass_register(CLASS_OBJ, c);
    hoa_decoder_class = c;
}

static void *hoa_decoder_3d_new(t_symbol *s, int argc, t_atom *argv)
{
    ulong order = 1;
    t_symbol* mode = gensym("regular");
    ulong number_of_channels = 4;
    t_hoa_decoder_3d *x = (t_hoa_decoder_3d *)eobj_new(hoa_decoder_3d_class);
    t_binbuf *d         = binbuf_via_atoms(argc,argv);

    if(x && d)
    {
        if(argc && argv && atom_gettype(argv) == A_LONG)
        {
            order = ulong(pd_clip_minmax(atom_getlong(argv), 1, 10));
            number_of_channels = (order+1)*(order+1);
        }
        if(argc > 1 && argv+1 && atom_gettype(argv+1) == A_SYM)
            mode = atom_getsym(argv+1);
        if(argc > 2 && argv+2 && atom_gettype(argv+2) == A_LONG)
            number_of_channels = ulong(pd_clip_min(atom_getlong(argv+2), 1));

        if(mode == gensym("irregular"))
        {
            x->f_decoder = new Decoder<Hoa3d, t_sample>::Regular(order, number_of_channels);
            x->f_mode = mode;
        }
        else if(mode == gensym("binaural"))
        {
            x->f_decoder = new Decoder<Hoa3d, t_sample>::Binaural(order);
            x->f_mode = mode;
        }
        else
        {
            x->f_decoder = new Decoder<Hoa3d, t_sample>::Regular(order, number_of_channels);
            x->f_mode = mode;
        }

        eobj_dspsetup(x, long(x->f_decoder->getNumberOfHarmonics()), long(x->f_decoder->getNumberOfPlanewaves()));
        x->f_ins = Signal<t_sample>::alloc(x->f_decoder->getNumberOfHarmonics() * HOA_MAXBLKSIZE);
        x->f_outs= Signal<t_sample>::alloc(x->f_decoder->getNumberOfPlanewaves() * HOA_MAXBLKSIZE);
        ebox_attrprocess_viabinbuf(x, d);
        
        return x;
    }

    return NULL;
}

static void hoa_decoder_3d_perform_regular(t_hoa_decoder_3d *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_ins+i, ulong(numins));
    }
    for(long i = 0; i < sampleframes; i++)
    {
        (static_cast<Decoder<Hoa3d, t_sample>::Regular*>(x->f_decoder))->process(x->f_ins + numins * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_decoder_3d_perform_binaural(t_hoa_decoder_3d *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    (static_cast<Decoder<Hoa3d, t_sample>::Binaural*>(x->f_decoder))->processBlock((const t_sample**)ins, outs);
}

static void hoa_decoder_3d_dsp(t_hoa_decoder_3d *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    x->f_decoder->computeRendering(ulong(maxvectorsize));
    if(x->f_mode == gensym("binaural"))
    {
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_decoder_3d_perform_binaural, 0, NULL);
    }
    else
    {
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_decoder_3d_perform_regular, 0, NULL);
    }
}

static t_pd_err hoa_decoder_3d_angles_set(t_hoa_decoder_3d *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        int dspState = canvas_suspend_dsp();
        for(long i = 0, j = 0; j < argc && i < (long)x->f_decoder->getNumberOfPlanewaves() * 2; j++)
        {
            if(atom_gettype(argv+j) == A_FLOAT)
            {
                if(j%2)
                {
                    x->f_decoder->setPlanewaveElevation(ulong(i), atom_getfloat(argv+j) / 360. * HOA_2PI);
                    i++;
                }
                else
                {
                    x->f_decoder->setPlanewaveAzimuth(ulong(i), atom_getfloat(argv+j) / 360. * HOA_2PI);
                }

            }
        }
        canvas_resume_dsp(dspState);
    }

    return 0;
}

static t_pd_err hoa_decoder_3d_angles_get(t_hoa_decoder_3d *x, void *attr, int* argc, t_atom **argv)
{
    *argc = int(x->f_decoder->getNumberOfPlanewaves() * 2);
    *argv = (t_atom *)malloc(size_t(*argc) * sizeof(t_atom));
    if(*argc && *argv)
    {
        for(ulong i = 0; i < x->f_decoder->getNumberOfPlanewaves(); i++)
        {
            atom_setfloat(*argv+i*2, x->f_decoder->getPlanewaveAzimuth(i) * 360. / HOA_2PI);
            atom_setfloat(*argv+i*2+1, x->f_decoder->getPlanewaveElevation(i) * 360. / HOA_2PI);
        }
    }
    else
    {
        *argc = 0;
        *argv = NULL;
    }
    return 0;
}

static t_pd_err hoa_decoder_3d_offset_set(t_hoa_decoder_3d *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        double ax, ay, az;
        int dspState = canvas_suspend_dsp();
        if(atom_gettype(argv) == A_FLOAT)
            ax = atom_getfloat(argv) / 360. * HOA_2PI;
        else
            ax = x->f_decoder->getPlanewavesRotationX();
        if(argc > 1 && atom_gettype(argv+1) == A_FLOAT)
            ay = atom_getfloat(argv+1) / 360. * HOA_2PI;
        else
            ay = x->f_decoder->getPlanewavesRotationY();
        if(argc > 2 &&  atom_gettype(argv+2) == A_FLOAT)
            az = atom_getfloat(argv+2) / 360. * HOA_2PI;
        else
            az = x->f_decoder->getPlanewavesRotationZ();
        x->f_decoder->setPlanewavesRotation(ax, ay, az);
        canvas_resume_dsp(dspState);
    }
    return 0;
}

static t_pd_err hoa_decoder_3d_offset_get(t_hoa_decoder_3d *x, void *attr, int* argc, t_atom **argv)
{
    *argc = 3;
    *argv = (t_atom *)malloc(size_t(*argc) * sizeof(t_atom));
    if(*argc && *argv)
    {
        atom_setfloat(*argv, x->f_decoder->getPlanewavesRotationX() / 360. * HOA_2PI);
        atom_setfloat(*argv+1, x->f_decoder->getPlanewavesRotationY() / 360. * HOA_2PI);
        atom_setfloat(*argv+2, x->f_decoder->getPlanewavesRotationZ() / 360. * HOA_2PI);
    }
    else
    {
        *argc = 0;
        *argv = NULL;
    }
    return 0;
}

static t_pd_err hoa_decoder_3d_crop_set(t_hoa_decoder_3d *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv && atom_gettype(argv) == A_FLOAT)
    {
        if(x->f_mode == gensym("binaural"))
        {
            int dspState = canvas_suspend_dsp();
            (static_cast<Decoder<Hoa3d, t_sample>::Binaural*>(x->f_decoder))->setCropSize(atom_getfloat(argv));
            canvas_resume_dsp(dspState);
        }
    }
    return 0;
}

static t_pd_err hoa_decoder_3d_crop_get(t_hoa_decoder_3d *x, void *attr, int* argc, t_atom **argv)
{
    *argc = 1;
    *argv = (t_atom *)malloc(size_t(*argc) * sizeof(t_atom));
    if(*argc && *argv)
    {
        if(x->f_mode == gensym("binaural"))
        {
            atom_setfloat(*argv, (static_cast<Decoder<Hoa3d, t_sample>::Binaural*>(x->f_decoder))->getCropSize());
        }
        else
        {
            atom_setfloat(*argv, 0);
        }
    }
    else
    {
        *argc = 0;
        *argv = NULL;
    }
    return 0;
}

static void hoa_decoder_3d_free(t_hoa_decoder_3d *x)
{
    eobj_dspfree(x);
    delete x->f_decoder;
    Signal<t_sample>::free(x->f_ins);
    Signal<t_sample>::free(x->f_outs);
}

extern "C" void setup_hoa0x2e3d0x2edecoder_tilde(void)
{
    t_eclass* c = eclass_new("hoa.3d.decoder~", (method)hoa_decoder_3d_new, (method)hoa_decoder_3d_free, (short)sizeof(t_hoa_decoder_3d), 0L, A_GIMME, 0);
    
    eclass_dspinit(c);
    eclass_addmethod(c, (method)hoa_decoder_3d_dsp,           "dsp",          A_CANT,  0);

    CLASS_ATTR_DOUBLE_VARSIZE	(c, "angles",0, t_hoa_decoder_3d, f_attrs, f_attrs, HOA_MAX_PLANEWAVES*2);
    CLASS_ATTR_ACCESSORS		(c, "angles", hoa_decoder_3d_angles_get, hoa_decoder_3d_angles_set);
    CLASS_ATTR_CATEGORY			(c, "offset", 0, "Planewaves");
    CLASS_ATTR_LABEL			(c, "angles", 0, "Angles of Loudspeakers");
    CLASS_ATTR_SAVE             (c, "angles", 0);

    CLASS_ATTR_DOUBLE_ARRAY     (c, "offset", 0, t_hoa_decoder_3d, f_attrs, 3);
    CLASS_ATTR_ACCESSORS		(c, "offset", hoa_decoder_3d_offset_get, hoa_decoder_3d_offset_set);
    CLASS_ATTR_CATEGORY			(c, "offset", 0, "Planewaves");
    CLASS_ATTR_LABEL            (c, "offset", 0, "Offset of Channels");
    CLASS_ATTR_SAVE             (c, "offset", 0);
    
    CLASS_ATTR_LONG             (c, "crop", 0, t_hoa_decoder, f_attrs);
    CLASS_ATTR_ACCESSORS		(c, "crop", hoa_decoder_3d_crop_get, hoa_decoder_3d_crop_set);
    CLASS_ATTR_CATEGORY			(c, "crop", 0, "Planewaves");
    CLASS_ATTR_LABEL            (c, "crop", 0, "Crop of the Responses");
    CLASS_ATTR_SAVE             (c, "crop", 0);

    eclass_register(CLASS_OBJ, c);
    hoa_decoder_3d_class = c;
}

