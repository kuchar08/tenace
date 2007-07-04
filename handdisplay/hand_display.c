/*
 *  hand display - bridge hand widget for GTK+
 *  Copyright (C) 2007 Christoph Berg <cb@df7cb.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <assert.h>
#include <glib/gtypes.h>
#include <gtk/gtk.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>

#include "hand_display.h"

/* static data */

//static GtkWidgetClass *parent_class = NULL;
static const int card_label[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
			13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
			26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
			39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
static RsvgHandle *rsvg = NULL;
static cairo_surface_t *surface = NULL;
static cairo_pattern_t *pattern = NULL;

/* internal functions */

static int
which_card (HandDisplay *handdisp, double x, double y)
{
	int c;
	for (c = 0; c < 52; c++) {
		if (handdisp->cards[c] && x >= handdisp->l[c] && x <= handdisp->r[c]
				       && y >= handdisp->t[c] && y <= handdisp->b[c]) {
			return c;
			break;
		}
	}
	return -1;
}

static char *
overtricks (int i)
{
	static char buf[4];
	if (i == 0)
		snprintf(buf, 3, "=");
	else
		snprintf(buf, 3, "%+d", i);
	return buf;
}

static void
render_card (cairo_t *cr, double x, double y, int c)
{
	static int init = 0;
	static GdkPixbuf *cards[52];
	static int width, height;

	assert (0 <= c && c < 52);
	
	if (!init) {
		printf("Initializing card pixmaps\n");
		GError *error = NULL;
		//char *fname = "/home/cb/projects/bridge/svg-cards/SVG-cards-2.0.1/svg-cards.svg";
		char *fname = "/home/cb/projects/bridge/tenace/bonded.svg";
		GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_size (fname, 40 * 13, 500, &error);
		if (!pb) {
			printf ("moo: %s.\n", error->message);
			exit (1);
		}
		width = gdk_pixbuf_get_width (pb) / 13;
		height = gdk_pixbuf_get_height (pb) / 5;
		int i;
		for (i = 0; i < 52; i++) {
			cards[i] = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
			if (!cards[i]) {
				printf ("cards[i]\n");
				exit (1);
			}
			int col = (i + 1) % 13;
			int row = i / 13;
			gdk_pixbuf_copy_area (pb, width * col, height * row, width, height,
				cards[i], 0, 0);
		}
		init = 1;
	}
	//cairo_save (cr);
	//cairo_translate (cr, x, y);
	//cairo_rectangle (cr, 0, 0, width, height);
	//cairo_clip (cr);
	//cairo_move_to (cr, 0, 0);
	//cairo_set_antialias (cr, CAIRO_ANTIALIAS_GRAY);
	gdk_cairo_set_source_pixbuf (cr, cards[c], x, y);
	cairo_paint (cr);
	//cairo_restore (cr);
	return;
}

static void
draw (GtkWidget *hand, cairo_t *cr)
{
	double l, r, t, b, x, y;
	int suit;
	HandDisplay *handdisp = HAND_DISPLAY(hand);
	cairo_text_extents_t extents;

	l = hand->allocation.x;
	r = l + hand->allocation.width;
	t = hand->allocation.y;
	b = t + hand->allocation.height;

	/* compute cached best card score for this hand */
	if (handdisp->best_card_score == HAND_DISPLAY_NO_SCORE) {
		int c;
		handdisp->best_card_score = handdisp->card_score_neg ? 14 : -14;
		for (c = 0; c < 52; c++) {
			if (handdisp->card_score_neg ?
				(handdisp->card_score[c] < handdisp->best_card_score) :
				(handdisp->card_score[c] > handdisp->best_card_score))
					handdisp->best_card_score = handdisp->card_score[c];
		}
	}

	/* draw suit symbols */
	cairo_select_font_face (cr, "Symbol", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (cr, 20);
	char *suit_str[] = {"♣", "♦", "♥", "♠"};
	double suit_color[4][3] = {
		{ HAND_DISPLAY_SPADES_FONT   },
		{ HAND_DISPLAY_HEARTS_FONT   },
		{ HAND_DISPLAY_DIAMONDS_FONT },
		{ HAND_DISPLAY_CLUBS_FONT    }
	};

	//char *suit_str[] = {"C ", "D ", "H ", "S "};
	double suit_width = 0.0;
	for (suit = 0; suit < 4; suit++) {
		x = 4;
		y = ((double) hand->allocation.height * (3.8 - suit) / 4.0);
		cairo_move_to (cr, x, y);
		cairo_text_extents (cr, suit_str[suit], &extents);
		if (extents.x_advance > suit_width)
			suit_width = extents.x_advance;
		cairo_set_source_rgb (cr, suit_color[suit][0], suit_color[suit][1], suit_color[suit][2]);
		cairo_show_text (cr, suit_str[suit]);
	}

	/* draw cards */
	cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_BOLD);
	char *rank_str[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
	for (suit = 0; suit < 4; suit++) {
		x = 4 + suit_width;
		y = ((double) hand->allocation.height * (3.8 - suit) / 4.0);
		cairo_move_to (cr, x, y);

		int c;
		for (c = 13 * (suit + 1) - 1; c >= 13 * suit; c--) {
			if (handdisp->cards[c]) {
				render_card (cr, x, y - 60, c);
				cairo_text_extents (cr, rank_str[c % 13], &extents);
				handdisp->l[c] = x + extents.x_bearing;
				handdisp->r[c] = x + extents.x_bearing + extents.width;
				handdisp->t[c] = y + extents.y_bearing;
				handdisp->b[c] = y + extents.y_bearing + extents.height;
				if (c == handdisp->cur_focus) {
					cairo_set_source_rgb (cr, HAND_DISPLAY_FOCUS_BG);
					cairo_rectangle (cr, handdisp->l[c] - 1, handdisp->t[c] - 1,
						extents.width + 2, extents.height + 2);
					cairo_fill (cr);
				}
				if (handdisp->card_score[c] == HAND_DISPLAY_NO_SCORE) {
					if (handdisp->cards[c] == HAND_DISPLAY_CARD)
						cairo_set_source_rgb (cr, HAND_DISPLAY_FONT);
					else if (handdisp->cards[c] == HAND_DISPLAY_GREY_CARD)
						cairo_set_source_rgb (cr, HAND_DISPLAY_GREY_FONT);
					else if (handdisp->cards[c] == HAND_DISPLAY_OLD_CARD)
						cairo_set_source_rgb (cr, HAND_DISPLAY_OLD_FONT);
				} else {
					/* invert colors if the score is for the opps */
					if (handdisp->card_score_neg ^ (handdisp->card_score[c] >= 0))
						if (handdisp->best_card_score == handdisp->card_score[c])
							cairo_set_source_rgb (cr, HAND_DISPLAY_BEST_POS_FONT);
						else
							cairo_set_source_rgb (cr, HAND_DISPLAY_POS_FONT);
					else
						if (handdisp->best_card_score == handdisp->card_score[c])
							cairo_set_source_rgb (cr, HAND_DISPLAY_BEST_NEG_FONT);
						else
							cairo_set_source_rgb (cr, HAND_DISPLAY_NEG_FONT);
				}
				cairo_move_to (cr, x, y);
				cairo_show_text (cr, rank_str[c % 13]);
				x += extents.x_advance; y += extents.y_advance;
			}
		}
	}

	/* draw card scores */
	//cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
			//CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 10);
	int c;
	for (c = 51; c >= 0; c--) {
		char *buf;
		if (handdisp->card_score[c] != HAND_DISPLAY_NO_SCORE) {
			buf = overtricks (handdisp->card_score[c]);
			cairo_text_extents (cr, buf, &extents);
			/*
			cairo_move_to (cr, handdisp->l[c] - 1, handdisp->b[c] + 1);
			cairo_rel_line_to (cr, 0, -extents.height - 2);
			cairo_rel_line_to (cr, extents.width + 2, 0);
			cairo_rel_line_to (cr, 0, extents.height + 2);
			cairo_set_source_rgb (cr, 0.0, 0.0, 1.0);
			cairo_fill (cr);
			*/

			cairo_move_to (cr, handdisp->r[c] - extents.x_advance, handdisp->b[c]);
			cairo_set_source_rgb (cr, HAND_DISPLAY_DD_FONT);
			cairo_show_text (cr, buf);
		}
	}
}

static void
redraw_card (GtkWidget *hand, int card)
{
	HandDisplay *handdisp = HAND_DISPLAY(hand);
	GdkRectangle rect;
	rect.x = handdisp->l[card] - 2;
	rect.y = handdisp->t[card] - 2;
	rect.width = handdisp->r[card] - handdisp->l[card] + 4;
	rect.height = handdisp->b[card] - handdisp->t[card] + 4;
	gdk_window_invalidate_rect (hand->window, &rect, FALSE);
}

/* private callbacks */

static gboolean
hand_display_motion (GtkWidget *hand, GdkEventMotion *event)
{
	HandDisplay *handdisp = HAND_DISPLAY(hand);
	int card = which_card(handdisp, event->x, event->y);
	if (handdisp->cur_focus != card) {
		if (handdisp->cur_focus != -1) {
			redraw_card (hand, handdisp->cur_focus);
			//printf("%c%d left\n", "CDHS"[handdisp->cur_focus / 13], handdisp->cur_focus % 13 + 2);
			g_signal_emit_by_name (handdisp, "card-leave", card_label + handdisp->cur_focus);
		}
		handdisp->cur_focus = card;
		if (card != -1) {
			redraw_card (hand, card);
			//printf("%c%d entered\n", "CDHS"[card / 13], card % 13 + 2);
			g_signal_emit_by_name (handdisp, "card-enter", card_label + card);
		}
	}
	gdk_window_get_pointer(hand->window, NULL, NULL, NULL); /* request more pointer hints */
	return FALSE;
}

static gboolean
hand_display_leave (GtkWidget *hand, GdkEventCrossing *event)
{
	HandDisplay *handdisp = HAND_DISPLAY(hand);
	if (handdisp->cur_focus != -1) {
		redraw_card (hand, handdisp->cur_focus);
		//printf("%c%d left\n", "CDHS"[handdisp->cur_focus / 13], handdisp->cur_focus % 13 + 2);
		g_signal_emit_by_name (handdisp, "card-leave", card_label + handdisp->cur_focus);
		handdisp->cur_focus = -1;
	}
	return FALSE;
}

static gboolean
hand_display_button_press (GtkWidget *hand, GdkEventButton *event)
{
	HandDisplay *handdisp = HAND_DISPLAY(hand);
	int card = which_card(handdisp, event->x, event->y);

	handdisp->cur_focus = card;
	if (handdisp->cur_focus == -1)
		return FALSE;
	redraw_card (hand, card);

	//printf("%c%d click type %d\n", "CDHS"[card / 13], card % 13 + 2, event->type);

	if (event->type == GDK_BUTTON_PRESS) {
		handdisp->cur_click = card;
	} else if (event->type == GDK_BUTTON_RELEASE && handdisp->cur_click == card) {
		g_signal_emit_by_name (handdisp, "card-clicked", card_label + card);
	}
	return FALSE;
}

gboolean DNDDragMotionCB(
        GtkWidget *hand, GdkDragContext *dc,
        gint x, gint y, guint t,
        gpointer data
)
{
	//printf ("DNDDragMotionCB %p %p %p x%d y%d t%d\n", hand, dc, data, x, y, t);
	HandDisplay *handdisp = HAND_DISPLAY(hand);
	int card = which_card(handdisp, x, y);
	handdisp->cur_focus = card;
	if (card != -1)
		redraw_card (hand, card);
	return FALSE;
}

static void
hand_display_realize (GtkWidget *widget)
{
	GdkWindowAttr attributes;
	gint attributes_mask;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (IS_HAND_DISPLAY (widget));

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.event_mask = gtk_widget_get_events (widget) |
		GDK_EXPOSURE_MASK | GDK_LEAVE_NOTIFY_MASK |
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
		GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);

	widget->style = gtk_style_attach (widget->style, widget->window);

	gdk_window_set_user_data (widget->window, widget);

	gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);
}

static gboolean
hand_display_expose (GtkWidget *hand, GdkEventExpose *event)
{
	cairo_t *cr;

	/* get a cairo_t */
	cr = gdk_cairo_create (hand->window);

	/* set a clip region for the expose event */
	cairo_rectangle (cr, event->area.x, event->area.y,
			event->area.width, event->area.height);
	cairo_clip (cr);
	draw (hand, cr);
	cairo_destroy (cr);

	return FALSE;
}

static void
hand_display_size_request (GtkWidget *hand, GtkRequisition *requisition)
{
	requisition->width = 100;
	requisition->height = 100;
}

static void
hand_display_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (IS_HAND_DISPLAY (widget));
	g_return_if_fail (allocation != NULL);

	widget->allocation = *allocation;
	if (GTK_WIDGET_REALIZED (widget)) {
		gdk_window_move_resize (widget->window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);
	}
}

/* initializers */

static void
hand_display_class_init (HandDisplayClass *class)
{
	//GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	//object_class = GTK_OBJECT_CLASS (class);
	widget_class = GTK_WIDGET_CLASS (class);

	//parent_class = gtk_type_class (gtk_widget_get_type ());

	//object_class->destroy = hand_display_destroy;

	widget_class->realize = hand_display_realize;
	widget_class->expose_event = hand_display_expose;
	widget_class->size_request = hand_display_size_request;
	widget_class->size_allocate = hand_display_size_allocate;
	widget_class->motion_notify_event = hand_display_motion;
	widget_class->leave_notify_event = hand_display_leave;
	widget_class->button_press_event = hand_display_button_press;
	widget_class->button_release_event = hand_display_button_press;

	g_signal_new ("card-clicked",
			TYPE_HAND_DISPLAY,
			G_SIGNAL_RUN_LAST,
			0 /* guint class_offset */,
			NULL /* GSignalAccumulator accumulator */,
			NULL /* gpointer accu_data */,
			g_cclosure_marshal_VOID__INT /* GSignalCMarshaller c_marshaller */,
			G_TYPE_NONE /* GType return_type */,
			1 /* guint n_params */,
			G_TYPE_INT);
	g_signal_new ("card-enter",
			TYPE_HAND_DISPLAY,
			G_SIGNAL_RUN_LAST,
			0 /* guint class_offset */,
			NULL /* GSignalAccumulator accumulator */,
			NULL /* gpointer accu_data */,
			g_cclosure_marshal_VOID__INT /* GSignalCMarshaller c_marshaller */,
			G_TYPE_NONE /* GType return_type */,
			1 /* guint n_params */,
			G_TYPE_INT);
	g_signal_new ("card-leave",
			TYPE_HAND_DISPLAY,
			G_SIGNAL_RUN_LAST,
			0 /* guint class_offset */,
			NULL /* GSignalAccumulator accumulator */,
			NULL /* gpointer accu_data */,
			g_cclosure_marshal_VOID__INT /* GSignalCMarshaller c_marshaller */,
			G_TYPE_NONE /* GType return_type */,
			1 /* guint n_params */,
			G_TYPE_INT);

	/*
	GError *error = NULL;
	rsvg = rsvg_handle_new_from_file ("/home/cb/projects/bridge/svg-cards/SVG-cards-2.0.1/svg-cards.svg", &error);
	if (!rsvg) {
		printf ("rsvg error: %s", error->message);
		exit (1);
	}

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 100, 100);
	printf("create\n");
	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
		printf ("moo.\n");
		exit (1);
	}

	cairo_t *cr = cairo_create (surface);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
		printf ("boo: %d.\n", cairo_status (cr));
		exit (1);
	}
	//rsvg_handle_render_cairo (rsvg, cr);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
		printf ("foo: %d.\n", cairo_status (cr));
		exit (1);
	}
	//cairo_stroke (cr);
	//pattern = cairo_pattern_create_for_surface (surface);
	*/
}

static void
hand_display_init (HandDisplay *handdisp)
{
	int i;
	for (i = 0; i < 52; i++) {
		handdisp->cards[i] = 0;
		handdisp->card_score[i] = -1;
		handdisp->l[i] = handdisp->r[i] = handdisp->t[i] = handdisp->b[i] = -1;
	}
	handdisp->card_score_neg = 0;
	handdisp->best_card_score = HAND_DISPLAY_NO_SCORE;

	handdisp->cur_focus = handdisp->cur_click = -1;
}

/* public interface */

GtkType
hand_display_get_type ()
{
	static GtkType hand_display_type = 0;

	if (!hand_display_type)
	{
		static const GtkTypeInfo hand_display_info =
		{
			"HandDisplay",
			sizeof (HandDisplay),
			sizeof (HandDisplayClass),
			(GtkClassInitFunc) hand_display_class_init,
			(GtkObjectInitFunc) hand_display_init,
			/* reserved_1 */ NULL,
			/* reserved_1 */ NULL,
			(GtkClassInitFunc) NULL
		};

		hand_display_type = gtk_type_unique (GTK_TYPE_WIDGET, &hand_display_info);
	}

	return hand_display_type;
}

GtkWidget *
hand_display_new (void)
{
	HandDisplay *handdisp = g_object_new (TYPE_HAND_DISPLAY, NULL);
	return GTK_WIDGET(handdisp);
}

void
hand_display_draw (GtkWidget *hand)
{
	gtk_widget_queue_draw_area (hand, 0, 0, hand->allocation.width, hand->allocation.height);
}

void
hand_display_set_card (HandDisplay *handdisp, int card, int val)
{
	assert (card >= 0 && card < 52);
	handdisp->cards[card] = val;
	handdisp->card_score[card] = HAND_DISPLAY_NO_SCORE;
}

void
hand_display_set_card_score (HandDisplay *handdisp, int card, int score)
{
	assert (card >= 0 && card < 52);
	handdisp->card_score[card] = score;
}

void
hand_display_set_card_score_neg (HandDisplay *handdisp, int neg)
{
	handdisp->card_score_neg = neg;
	handdisp->best_card_score = HAND_DISPLAY_NO_SCORE;
}
