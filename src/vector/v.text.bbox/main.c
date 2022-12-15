#include <stdio.h>
#include <stdlib.h>
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    
    struct GModule *module;
    struct Option *opt_in_map, *opt_out_map, *opt_where, *opt_layer, *opt_cats, *opt_text_col,
    *opt_margin, *opt_size_col, *opt_font_size, *opt_font;
    struct Flag *fl_m;
    
    const char *mapset;
    
    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("text"));
    G_add_keyword(_("bounding box"));
    module->description = _("Convert text bounding box to a polygon");
    
    opt_in_map = G_define_standard_option(G_OPT_V_INPUT);
    opt_in_map->required = YES;
    opt_in_map->guisection = _("Main");
    
    opt_out_map = G_define_standard_option(G_OPT_V_OUTPUT);
    opt_out_map->required = YES;
    opt_out_map->guisection = _("Main");
    
    opt_text_col = G_define_standard_option(G_OPT_DB_COLUMN);
    opt_text_col->required = YES;
    opt_text_col->description = _("Text column");
    opt_text_col->guisection = _("Main");
    
    opt_where = G_define_standard_option(G_OPT_DB_WHERE);
    opt_where->guisection = _("Selection");

    opt_layer = G_define_standard_option(G_OPT_V_FIELD);
    opt_layer->answer = "1";
    opt_layer->description = _("Use features only from specified layer");
    opt_layer->guisection = _("Selection");
    
    opt_cats = G_define_standard_option(G_OPT_V_CATS);
    opt_cats->guisection = _("Selection");
    
    opt_font = G_define_option();
    opt_font->key = "font";
    opt_font->type = TYPE_STRING;
    opt_font->required = NO;
    opt_font->description = _("Font name");
    opt_font->guisection = _("Font settings");

    opt_font_size = G_define_option();
    opt_font_size->key = "fontsize";
    opt_font_size->type = TYPE_DOUBLE;
    opt_font_size->required = NO;
    opt_font_size->options = "1-360";
    opt_font_size->label = _("Font size");
    opt_font_size->description = _("Default: 12");
    opt_font_size->guisection = _("Font settings");
    
    opt_size_col = G_define_standard_option(G_OPT_DB_COLUMN);
    opt_size_col->required = NO;
    opt_size_col->description = _("Font size column");
    opt_size_col->guisection = _("Font settings");
    
    opt_margin = G_define_option();
    opt_margin->key = "margin";
    opt_margin->type = TYPE_DOUBLE;
    opt_margin->required = NO;
    opt_margin->options = "0-100000";
    opt_margin->label = _("Text margin");
    opt_margin->description = _("Default: 2");
    opt_margin->guisection = _("Font settings");
    
    fl_m = G_define_flag();
    fl_m->key = 'm';
    fl_m->description = _("Sizes are in map units");
    fl_m->guisection = _("Font settings");
    
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);
    
    if ((mapset = G_find_vector2(opt_in_map->answer, "")) == NULL)
        G_fatal_error(_("Vector map <%s> not found"), opt_in_map->answer);
    
    G_fatal_error("Code not implemented!");
}
