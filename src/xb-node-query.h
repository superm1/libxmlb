/*
 * Copyright (C) 2018 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef __XB_NODE_QUERY_H
#define __XB_NODE_QUERY_H

G_BEGIN_DECLS

#include <glib-object.h>

#include "xb-query.h"
#include "xb-node.h"

GPtrArray	*xb_node_query			(XbNode		*self,
						 const gchar	*xpath,
						 guint		 limit,
						 GError		**error);
GPtrArray	*xb_node_query_full		(XbNode		*self,
						 XbQuery	*query,
						 GError		**error);
XbNode		*xb_node_query_first		(XbNode		*self,
						 const gchar	*xpath,
						 GError		**error);
const gchar	*xb_node_query_text		(XbNode		*self,
						 const gchar	*xpath,
						 GError		**error);
guint64		 xb_node_query_text_as_uint	(XbNode		*self,
						 const gchar	*xpath,
						 GError		**error);
const gchar	*xb_node_query_attr		(XbNode		*self,
						 const gchar	*xpath,
						 const gchar	*name,
						 GError		**error);
guint64		 xb_node_query_attr_as_uint	(XbNode		*self,
						 const gchar	*xpath,
						 const gchar	*name,
						 GError		**error);
gchar		*xb_node_query_export		(XbNode		*self,
						 const gchar	*xpath,
						 GError		**error);

G_END_DECLS

#endif /* __XB_NODE_QUERY_H */

