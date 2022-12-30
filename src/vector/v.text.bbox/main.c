/*****************************************************************************
 *
 * MODULE:       v.text.bbox
 * AUTHOR(S):    Maris Nartiss
 *               Based on d.vect code by:
 *               CERL, Radim Blazek, others
 *               Updated to GRASS7 by Martin Landa <landa.martin gmail.com>
 *               Support for vector legend by Adam Laza <ad.laza32 gmail.com >
 * PURPOSE:      Create a polygon around vector attribute label
 * COPYRIGHT:    (C) 2022 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <grass/config.h>
#include <grass/dbmi.h>
#include <grass/display.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vector.h>
#include <stdio.h>
#include <stdlib.h>

#define LCENTER 0
#define LLEFT 1
#define LRIGHT 2
#define LBOTTOM 3
#define LTOP 4

typedef struct {
  int field;
  int size;
  double padding;
  const char *font;
  const char *enc;
  const char *font_col;
  const char *size_col;
  int xref, yref;
  struct Map_info Out;
  struct field_info *Fi;
  dbDriver *Driver;
} LATTR;

int display_attr(struct Map_info *, char *, struct cat_list *, LATTR *, int);
void options_to_lattr(LATTR *, const char *, int, double, const char *,
                      const char *, const char *, const char *, const char *,
                      const char *, const char *);

int main(int argc, char *argv[]) {
  struct GModule *module;
  struct Option *opt_in_map, *opt_out_map, *opt_where, *opt_layer, *opt_cats,
      *opt_text_col, *opt_padding, *opt_size_col, *opt_font_col, *opt_font_size,
      *opt_font, *opt_enc, *opt_xref, *opt_yref;

  const char *mapset;

  struct cat_list *Clist;
  LATTR lattr;
  struct Map_info Map;
  struct Cell_head window;
  struct bound_box vbbox;
  int chcat = 0, ret;
  int level;
  int pxsize;

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
  opt_text_col->key = "label_col";
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
  opt_font_size->type = TYPE_INTEGER;
  opt_font_size->required = NO;
  opt_font_size->answer = "100";
  opt_font_size->options = "1-100000";
  opt_font_size->label = _("Font size in map units");
  opt_font_size->description = _("Default: 100");
  opt_font_size->guisection = _("Font settings");

  opt_size_col = G_define_standard_option(G_OPT_DB_COLUMN);
  opt_size_col->key = "size_col";
  opt_size_col->required = NO;
  opt_size_col->description = _("Font size column");
  opt_size_col->guisection = _("Font settings");

  opt_font_col = G_define_standard_option(G_OPT_DB_COLUMN);
  opt_font_col->key = "font_col";
  opt_font_col->required = NO;
  opt_font_col->description = _("Font column");
  opt_font_col->guisection = _("Font settings");

  opt_enc = G_define_option();
  opt_enc->key = "encoding";
  opt_enc->type = TYPE_STRING;
  opt_enc->description = _("Text encoding");
  opt_enc->guisection = _("Font settings");

  opt_padding = G_define_option();
  opt_padding->key = "padding";
  opt_padding->type = TYPE_DOUBLE;
  opt_padding->required = NO;
  opt_padding->options = "0-100000";
  opt_padding->answer = "2";
  opt_padding->label = _("Text padding");
  opt_padding->description = _("Default: 2");
  opt_padding->guisection = _("Position");

  opt_xref = G_define_option();
  opt_xref->key = "xref";
  opt_xref->type = TYPE_STRING;
  opt_xref->answer = "left";
  opt_xref->options = "left,center,right";
  opt_xref->description = _("Label horizontal justification");
  opt_xref->guisection = _("Position");

  opt_yref = G_define_option();
  opt_yref->key = "yref";
  opt_yref->type = TYPE_STRING;
  opt_yref->answer = "center";
  opt_yref->options = "top,center,bottom";
  opt_yref->description = _("Label vertical justification");
  opt_yref->guisection = _("Position");

  if (G_parser(argc, argv))
    exit(EXIT_FAILURE);

  if ((mapset = G_find_vector2(opt_in_map->answer, "")) == NULL)
    G_fatal_error(_("Vector map <%s> not found"), opt_in_map->answer);

  options_to_lattr(&lattr, opt_layer->answer, atoi(opt_font_size->answer),
                   atof(opt_padding->answer), opt_out_map->answer,
                   opt_font->answer, opt_enc->answer, opt_font_col->answer,
                   opt_size_col->answer, opt_xref->answer, opt_yref->answer);

  Clist = Vect_new_cat_list();
  Clist->field = atoi(opt_layer->answer);
  level = Vect_open_old2(&Map, opt_in_map->answer, "", opt_layer->answer);
  ret = Vect_get_map_box(&Map, &vbbox);
  if (ret == 0)
    G_fatal_error(_("Could not get the bounding box of map <%s>"),
                  opt_in_map->answer);

  G_get_set_window(&window);
  window.north = vbbox.N;
  window.south = vbbox.S;
  window.west = vbbox.W;
  window.east = vbbox.E;
  window.top = vbbox.T;
  window.bottom = vbbox.B;
  G_set_window(&window);
  pxsize = abs(ceil(window.north - window.south));
  if (pxsize < 100)
    pxsize = 100;

  chcat = 0;
  if (opt_where->answer) {
    int ncat;
    int *cats;
    struct field_info *fi;
    dbDriver *driver;
    if (Clist->field < 1)
      G_fatal_error(_("Option <%s> must be > 0"), opt_layer->key);
    chcat = 1;
    fi = Vect_get_field(&Map, lattr.field);
    if (fi == NULL)
      return 1;

    driver = db_start_driver_open_database(fi->driver, fi->database);
    if (driver == NULL)
      G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                    fi->database, fi->driver);

    ncat = db_select_int(driver, fi->table, fi->key, opt_where->answer, &cats);

    db_close_database(driver);
    db_shutdown_driver(driver);

    Vect_array_to_cat_list(cats, ncat, Clist);
  } else if (opt_cats->answer) {
    if (Clist->field < 1)
      G_fatal_error(_("Option <%s> must be > 0"), opt_layer->key);
    chcat = 1;
    ret = Vect_str_to_cat_list(opt_cats->answer, Clist);
    if (ret > 0)
      G_warning(n_("%d error in cat option", "%d errors in cat option", ret),
                ret);
  }

  char buff[512];
  sprintf(buff, "GRASS_RENDER_IMMEDIATE=cairo");
  putenv(G_store(buff));
  sprintf(buff, "GRASS_RENDER_WIDTH=%d", pxsize);
  putenv(G_store(buff));
  sprintf(buff, "GRASS_RENDER_HEIGHT=%d", pxsize);
  putenv(G_store(buff));

  D_open_driver();
  D_setup(0);
  D_set_reduction(1.0);
  display_attr(&Map, opt_text_col->answer, Clist, &lattr, chcat);

  D_close_driver();

  Vect_close(&Map);
  Vect_destroy_cat_list(Clist);
}

void options_to_lattr(LATTR *lattr, const char *layer, int size, double padding,
                      const char *out_map, const char *font,
                      const char *encoding, const char *font_col,
                      const char *size_col, const char *xref,
                      const char *yref) {
  dbString sql;
  char buf[2000], buf2[2000], buf3[2000];

  if (layer)
    lattr->field = atoi(layer);
  else
    lattr->field = 1;

  lattr->size = size;
  lattr->padding = padding;
  lattr->font = font;
  lattr->enc = encoding;
  if (font_col)
    lattr->font_col = font_col;
  else
    lattr->font_col = NULL;
  if (size_col)
    lattr->size_col = size_col;
  else
    lattr->size_col = NULL;
  if (xref) {
    switch (xref[0]) {
    case 'l':
      lattr->xref = LLEFT;
      break;
    case 'c':
      lattr->xref = LCENTER;
      break;
    case 'r':
      lattr->xref = LRIGHT;
      break;
    }
  } else
    lattr->xref = LCENTER;

  if (yref) {
    switch (yref[0]) {
    case 't':
      lattr->yref = LTOP;
      break;
    case 'c':
      lattr->yref = LCENTER;
      break;
    case 'b':
      lattr->yref = LBOTTOM;
      break;
    }
  } else
    lattr->yref = LCENTER;

  /* Create output map + its attribute table */
  if (0 > Vect_open_new(&lattr->Out, out_map, 0)) {
    G_fatal_error(_("Unable to create vector map <%s>"), out_map);
  }
  Vect_hist_command(&lattr->Out);
  lattr->Fi = Vect_default_field_info(&lattr->Out, 1, NULL, GV_1TABLE);
  Vect_map_add_dblink(&lattr->Out, lattr->Fi->number, lattr->Fi->name,
                      lattr->Fi->table, lattr->Fi->key, lattr->Fi->database,
                      lattr->Fi->driver);

  lattr->Driver = db_start_driver_open_database(
      lattr->Fi->driver, Vect_subst_var(lattr->Fi->database, &lattr->Out));
  if (lattr->Driver == NULL)
    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                  lattr->Fi->database, lattr->Fi->driver);
  db_set_error_handler_driver(lattr->Driver);
  db_init_string(&sql);
  if (lattr->font_col)
    sprintf(buf2, ", font varchar(255)");
  else
    sprintf(buf2, "");
  if (lattr->size_col)
    sprintf(buf3, ", size varchar(255)");
  else
    sprintf(buf3, "");
  sprintf(buf, "create table %s (%s integer, label varchar(255)%s%s)",
          lattr->Fi->table, lattr->Fi->key, buf2, buf3);

  db_set_string(&sql, buf);
  if (db_execute_immediate(lattr->Driver, &sql) != DB_OK) {
    G_fatal_error(_("Unable to create table: %s"), db_get_string(&sql));
  }

  if (db_create_index2(lattr->Driver, lattr->Fi->table, lattr->Fi->key) !=
      DB_OK)
    G_warning(_("Unable to create index"));

  if (db_grant_on_table(lattr->Driver, lattr->Fi->table, DB_PRIV_SELECT,
                        DB_GROUP | DB_PUBLIC) != DB_OK)
    G_fatal_error(_("Unable to grant privileges on table <%s>"),
                  lattr->Fi->table);
}

int display_attr(struct Map_info *Map, char *attrcol, struct cat_list *Clist,
                 LATTR *lattr, int chcat) {
  int i, ltype, more;
  double X, Y;
  double size;
  char font[2000];
  struct line_pnts *in_Points, *out_Points;
  struct line_cats *in_Cats, *out_Cats;
  int cat;
  char buf[2000], buf2[2000], buf3[2000];
  struct field_info *fi;
  dbDriver *driver;
  dbString stmt, valstr, text, sql;
  dbCursor cursor;
  dbTable *table;
  dbColumn *column;

  G_debug(2, "attr()");

  if (attrcol == NULL || *attrcol == '\0') {
    G_fatal_error(_("attrcol not specified"));
  }

  db_init_string(&stmt);
  db_init_string(&valstr);
  db_init_string(&text);
  db_init_string(&sql);

  fi = Vect_get_field(Map, lattr->field);
  if (fi == NULL)
    return 1;

  driver = db_start_driver_open_database(fi->driver, fi->database);
  if (driver == NULL)
    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                  fi->database, fi->driver);

  if (lattr->enc)
    D_encoding(lattr->enc);

  in_Points = Vect_new_line_struct();
  in_Cats = Vect_new_cats_struct();
  out_Points = Vect_new_line_struct();
  out_Cats = Vect_new_cats_struct();
  Vect_rewind(Map);
  while (1) {
    ltype = Vect_read_next_line(Map, in_Points, in_Cats);
    if (ltype == -1)
      G_fatal_error(_("Unable to read vector map"));
    else if (ltype == -2) /* EOF */
      break;

    if (!(ltype & GV_POINT || ltype & GV_LINE || ltype & GV_CENTROID))
      continue;

    if (chcat) {
      int found = 0;

      for (i = 0; i < in_Cats->n_cats; i++) {
        if (in_Cats->field[i] == Clist->field &&
            Vect_cat_in_cat_list(in_Cats->cat[i], Clist)) {
          found = 1;
          break;
        }
      }
      if (!found)
        continue;
    } else if (Clist->field > 0) {
      int found = 0;

      for (i = 0; i < in_Cats->n_cats; i++) {
        if (in_Cats->field[i] == Clist->field) {
          found = 1;
          break;
        }
      }
      /* lines with no category will be displayed */
      if (in_Cats->n_cats > 0 && !found)
        continue;
    }

    if (Vect_cat_get(in_Cats, lattr->field, &cat)) {
      int ncats = 0;

      /* Read attribute from db */
      db_free_string(&text);
      for (i = 0; i < in_Cats->n_cats; i++) {
        int nrows;

        if (in_Cats->field[i] != lattr->field)
          continue;
        db_init_string(&stmt);
        if (lattr->size_col) {
          sprintf(buf2, ", %s", lattr->size_col);
        } else
          sprintf(buf2, "");
        if (lattr->font_col) {
          sprintf(buf3, ", %s", lattr->font_col);
        } else
          sprintf(buf3, "");
        sprintf(buf, "select %s%s%s from %s where %s = %d", attrcol, buf2, buf3,
                fi->table, fi->key, in_Cats->cat[i]);
        G_debug(2, "SQL: %s", buf);
        db_append_string(&stmt, buf);

        if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) !=
            DB_OK)
          G_fatal_error(_("Unable to open select cursor: '%s'"),
                        db_get_string(&stmt));

        nrows = db_get_num_rows(&cursor);

        if (ncats > 0)
          db_append_string(&text, "/");

        if (nrows > 0) {
          table = db_get_cursor_table(&cursor);
          if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
            continue;

          column = db_get_table_column(table, 0); /* first column */
          db_convert_column_value_to_string(column, &valstr);
          db_append_string(&text, db_get_string(&valstr));
          if (lattr->size_col) {
            column = db_get_table_column(table, 1);
            db_convert_column_value_to_string(column, &valstr);
            size = atof(db_get_string(&valstr));
          } else
            size = lattr->size;
          /* 0.7 is random value chosen by trial and error */
          D_text_size(size * 0.7, size * 0.7);
          if (lattr->font_col) {
            column = db_get_table_column(table, 2);
            db_convert_column_value_to_string(column, &valstr);
            sprintf(font, "%s", db_get_string(&valstr));
          } else
            sprintf(font, "%s", lattr->font);
          D_font(font);
        } else {
          G_warning(_("No attribute found for cat %d: %s"), cat,
                    db_get_string(&stmt));
        }

        db_close_cursor(&cursor);
        ncats++;
      }

      if ((ltype & GV_POINTS) || in_Points->n_points == 1)
      /* point/centroid or line/boundary with one coor */
      {
        X = in_Points->x[0];
        Y = in_Points->y[0];
      } else if (in_Points->n_points == 2) { /* line with two coors */
        X = (in_Points->x[0] + in_Points->x[1]) / 2;
        Y = (in_Points->y[0] + in_Points->y[1]) / 2;
      } else {
        i = in_Points->n_points / 2;
        X = in_Points->x[i];
        Y = in_Points->y[i];
      }

      int Xoffset, Yoffset;
      double xarr[5], yarr[5];
      double T, B, L, R;

      X = X + D_get_d_to_u_xconv() * 0.5 * size * 0.7;
      Y = Y + D_get_d_to_u_yconv() * 1.5 * size * 0.7;

      D_pos_abs(X, Y);
      D_get_text_box(db_get_string(&text), &T, &B, &L, &R);

      /* Expand border 1/2 of text size */
      T = T - D_get_d_to_u_yconv() + lattr->padding;
      B = B + D_get_d_to_u_yconv() - lattr->padding;
      L = L - D_get_d_to_u_xconv() - lattr->padding;
      R = R + D_get_d_to_u_xconv() + lattr->padding;

      Xoffset = 0;
      Yoffset = 0;
      if (lattr->xref == LCENTER)
        Xoffset = -(R - L) / 2;
      if (lattr->xref == LRIGHT)
        Xoffset = -(R - L);
      if (lattr->yref == LCENTER)
        Yoffset = -(B - T) / 2;
      if (lattr->yref == LBOTTOM)
        Yoffset = -(B - T);

      xarr[0] = xarr[1] = xarr[4] = L + Xoffset;
      xarr[2] = xarr[3] = R + Xoffset;
      yarr[0] = yarr[3] = yarr[4] = B + Yoffset;
      yarr[1] = yarr[2] = T + Yoffset;

      Vect_copy_xyz_to_pnts(out_Points, xarr, yarr, 0, 5);
      Vect_write_line(&lattr->Out, GV_BOUNDARY, out_Points, out_Cats);
      Vect_reset_line(out_Points);
      Vect_reset_cats(out_Cats);
      Vect_append_point(out_Points, xarr[0] + (xarr[2] - xarr[0]) / 2,
                        yarr[0] + (yarr[2] - yarr[0]) / 2, 0.0);
      Vect_cat_set(out_Cats, 1, cat);
      Vect_write_line(&lattr->Out, GV_CENTROID, out_Points, out_Cats);

      if (lattr->size_col)
        sprintf(buf2, ", '%f'", size);
      else
        sprintf(buf2, "");
      if (lattr->font_col)
        sprintf(buf3, ", '%s'", font);
      else
        sprintf(buf3, "");

      sprintf(buf, "insert into %s values ( %d, '%s' %s%s )", lattr->Fi->table,
              cat, db_get_string(&text), buf2, buf3);
      if (db_set_string(&sql, buf) != DB_OK)
        G_fatal_error(_("Unable to fill attribute table"));

      G_debug(3, "SQL: %s", db_get_string(&sql));
      if (db_execute_immediate(lattr->Driver, &sql) != DB_OK) {
        G_fatal_error(_("Unable to insert new record: %s"),
                      db_get_string(&sql));
      }
    }
  }

  db_close_database_shutdown_driver(lattr->Driver);
  db_close_database_shutdown_driver(driver);
  Vect_destroy_line_struct(in_Points);
  Vect_destroy_cats_struct(in_Cats);
  Vect_destroy_line_struct(out_Points);
  Vect_destroy_cats_struct(out_Cats);
  Vect_build(&lattr->Out);
  Vect_close(&lattr->Out);

  return 0;
}
