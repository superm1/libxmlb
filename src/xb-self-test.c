/*
 * Copyright (C) 2018 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include <gio/gio.h>

#include "xb-builder.h"
#include "xb-builder-node.h"
#include "xb-machine.h"
#include "xb-opcode.h"
#include "xb-silo-export.h"
#include "xb-silo-private.h"
#include "xb-silo-query.h"

static void
xb_predicate_func (void)
{
	g_autoptr(XbSilo) silo = xb_silo_new ();
	struct {
		const gchar	*pred;
		const gchar	*str;
	} tests[] = {
		{ "'a'='b'",
		  "'a','b',eq()" },
		{ "@a='b'",
		  "'a',attr(),'b',eq()" },
		{ "@a=='b'",
		  "'a',attr(),'b',eq()" },
		{ "'a'<'b'",
		  "'a','b',lt()" },
		{ "999>=123",
		  "999,123,ge()" },
		{ "@a",
		  "'a',attr(),'(null)',ne()" },
		{ "'a'=",
		  "'a',eq()" },
		{ "='b'",
		  "'b',eq()" },
		{ "999=\'b\'",
		  "999,'b',eq()" },
		{ "text()=\'b\'",
		  "text(),'b',eq()" },
		{ "last()",
		  "last()" },
		{ "text()~='beef'",
		  "text(),'beef',contains()" },
		{ "@type~='dead'",
		  "'type',attr(),'dead',contains()" },
		{ "2",
		  "2,position(),eq()" },
		{ "text()=lower-case('firefox')",
		  "text(),'firefox',lower-case(),eq()" },
		/* sentinel */
		{ NULL, NULL }
	};
	const gchar *invalid[] = {
		"text(",
		"text((((((((((((((((((((text()))))))))))))))))))))",
		NULL
	};
	xb_machine_set_debug_flags (xb_silo_get_machine (silo),
				    XB_MACHINE_DEBUG_FLAG_SHOW_STACK);
	for (guint i = 0; tests[i].pred != NULL; i++) {
		g_autofree gchar *str = NULL;
		g_autoptr(GError) error = NULL;
		g_autoptr(GPtrArray) opcodes = NULL;

		g_debug ("testing %s", tests[i].pred);
		opcodes = xb_machine_parse (xb_silo_get_machine (silo), tests[i].pred, -1, &error);
		g_assert_no_error (error);
		g_assert_nonnull (opcodes);
		str = xb_machine_opcodes_to_string (xb_silo_get_machine (silo), opcodes);
		g_assert_nonnull (str);
		g_assert_cmpstr (str, ==, tests[i].str);
	}
	for (guint i = 0; invalid[i] != NULL; i++) {
		g_autoptr(GError) error = NULL;
		g_autoptr(GPtrArray) opcodes = NULL;
		opcodes = xb_machine_parse (xb_silo_get_machine (silo), invalid[i], -1, &error);
		g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA);
		g_assert_null (opcodes);
	}
}

static void
xb_builder_func (void)
{
	g_autofree gchar *str = NULL;
	g_autofree gchar *xml_new = NULL;
	g_autoptr(GBytes) bytes = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(XbSilo) silo = NULL;
	const gchar *xml =
		"<components origin=\"lvfs\">\n"
		"  <header type=\"&lt;&amp;&gt;\">\n"
		"    <csum type=\"sha1\">dead</csum>\n"
		"  </header>\n"
		"  <component type=\"desktop\" attr=\"value\">\n"
		"    <id>gimp.desktop</id>\n"
		"    <name>GIMP &amp; Friends</name>\n"
		"    <id>org.gnome.Gimp.desktop</id>\n"
		"  </component>\n"
		"  <component type=\"desktop\">\n"
		"    <id>gnome-software.desktop</id>\n"
		"  </component>\n"
		"  <component type=\"firmware\">\n"
		"    <id>org.hughski.ColorHug2.firmware</id>\n"
		"    <requires>\n"
		"      <bootloader>1.2.3</bootloader>\n"
		"    </requires>\n"
		"  </component>\n"
		"</components>\n";

	/* import from XML */
	silo = xb_silo_new_from_xml (xml, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* convert back to XML */
	str = xb_silo_to_string (silo, &error);
	g_assert_no_error (error);
	g_assert_nonnull (str);
	g_debug ("\n%s", str);
	xml_new = xb_silo_export (silo,
				  XB_NODE_EXPORT_FLAG_FORMAT_MULTILINE |
				  XB_NODE_EXPORT_FLAG_FORMAT_INDENT,
				  &error);
	g_assert_no_error (error);
	g_assert_nonnull (xml_new);
	g_print ("%s", xml_new);
	g_assert_cmpstr (xml, ==, xml_new);

	/* check size */
	bytes = xb_silo_get_bytes (silo);
	g_assert_cmpint (g_bytes_get_size (bytes), ==, 529);
}

static void
xb_builder_ensure_func (void)
{
	gboolean ret;
	g_autoptr(GBytes) bytes1 = NULL;
	g_autoptr(GBytes) bytes2 = NULL;
	g_autoptr(GBytes) bytes3 = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(GFile) file = NULL;
	g_autoptr(XbBuilder) builder = xb_builder_new ();
	g_autoptr(XbSilo) silo = NULL;
	const gchar *xml =
		"<components origin=\"lvfs\">\n"
		"  <header type=\"&lt;&amp;&gt;\">\n"
		"    <csum type=\"sha1\">dead</csum>\n"
		"  </header>\n"
		"  <component type=\"desktop\" attr=\"value\">\n"
		"    <id>gimp.desktop</id>\n"
		"    <name>GIMP &amp; Friends</name>\n"
		"    <id>org.gnome.Gimp.desktop</id>\n"
		"  </component>\n"
		"  <component type=\"desktop\">\n"
		"    <id>gnome-software.desktop</id>\n"
		"  </component>\n"
		"  <component type=\"firmware\">\n"
		"    <id>org.hughski.ColorHug2.firmware</id>\n"
		"    <requires>\n"
		"      <bootloader>1.2.3</bootloader>\n"
		"    </requires>\n"
		"  </component>\n"
		"</components>\n";

	/* import some XML */
	ret = xb_builder_import_xml (builder, xml, &error);
	g_assert_no_error (error);
	g_assert_true (ret);

	/* create file if it does not exist */
	file = g_file_new_for_path ("/tmp/temp.xmlb");
	g_file_delete (file, NULL, NULL);
	silo = xb_builder_ensure (builder, file,
				  XB_BUILDER_COMPILE_FLAG_NONE,
				  NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);
	bytes1 = xb_silo_get_bytes (silo);
	g_clear_object (&silo);

	/* recreate file if it is invalid */
	ret = g_file_replace_contents (file, "dave", 4, NULL, FALSE,
				       G_FILE_CREATE_NONE, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	silo = xb_builder_ensure (builder, file,
				  XB_BUILDER_COMPILE_FLAG_NONE,
				  NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);
	bytes2 = xb_silo_get_bytes (silo);
	g_assert (bytes1 != bytes2);
	g_clear_object (&silo);

	/* don't recreate file if perfectly valid */
	silo = xb_builder_ensure (builder, file,
				  XB_BUILDER_COMPILE_FLAG_NONE,
				  NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);
	bytes3 = xb_silo_get_bytes (silo);
	g_assert (bytes2 == bytes3);
	g_clear_object (&silo);
	g_clear_object (&builder);

	/* don't re-create for a new builder with the same XML added */
	builder = xb_builder_new ();
	ret = xb_builder_import_xml (builder, xml, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	silo = xb_builder_ensure (builder, file,
				  XB_BUILDER_COMPILE_FLAG_NONE,
				  NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);
}

static gboolean
xb_builder_upgrade_appstream_cb (XbBuilder *self, XbBuilderNode *bn, gpointer user_data, GError **error)
{
	if (g_strcmp0 (xb_builder_node_get_element (bn), "application") == 0) {
		GPtrArray *children = xb_builder_node_get_children (bn);
		g_autofree gchar *kind = NULL;
		for (guint i = 0; i < children->len; i++) {
			XbBuilderNode *bc = g_ptr_array_index (children, i);
			if (g_strcmp0 (xb_builder_node_get_element (bc), "id") == 0) {
				kind = g_strdup (xb_builder_node_get_attribute (bc, "type"));
				xb_builder_node_remove_attr (bc, "type");
				break;
			}
		}
		if (kind != NULL)
			xb_builder_node_set_attr (bn, "type", kind);
		xb_builder_node_set_element (bn, "component");
	} else if (g_strcmp0 (xb_builder_node_get_element (bn), "metadata") == 0) {
		xb_builder_node_set_element (bn, "custom");
	}
	return TRUE;
}

static void
xb_builder_node_vfunc_func (void)
{
	gboolean ret;
	g_autofree gchar *xml2 = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(XbBuilder) builder = xb_builder_new ();
	g_autoptr(XbSilo) silo = NULL;
	const gchar *xml =
		"  <application>\n"
		"    <id type=\"desktop\">gimp.desktop</id>\n"
		"  </application>\n";

	/* import some XML */
	ret = xb_builder_import_xml (builder, xml, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	xb_builder_add_node_func (builder, xb_builder_upgrade_appstream_cb, NULL, NULL);
	silo = xb_builder_compile (builder,
				   XB_BUILDER_COMPILE_FLAG_NONE,
				   NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* check the XML */
	xml2 = xb_silo_export (silo, XB_NODE_EXPORT_FLAG_NONE, &error);
	g_assert_no_error (error);
	g_assert_nonnull (xml2);
	g_print ("%s\n", xml2);
	g_assert_cmpstr ("<component type=\"desktop\">"
			 "<id>gimp.desktop</id>"
			 "</component>", ==, xml2);
}

static void
xb_builder_empty_func (void)
{
	gboolean ret;
	g_autofree gchar *str = NULL;
	g_autofree gchar *xml = NULL;
	g_autoptr(GBytes) bytes = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(GPtrArray) results = NULL;
	g_autoptr(XbBuilder) builder = xb_builder_new ();
	g_autoptr(XbSilo) silo2 = xb_silo_new ();
	g_autoptr(XbSilo) silo = NULL;

	/* import from XML */
	silo = xb_builder_compile (builder, XB_BUILDER_COMPILE_FLAG_NONE, NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* check size */
	bytes = xb_silo_get_bytes (silo);
	g_assert_cmpint (g_bytes_get_size (bytes), ==, 32);

	/* try to dump */
	str = xb_silo_to_string (silo, &error);
	g_assert_no_error (error);
	g_assert_nonnull (str);
	g_debug ("%s", str);

	/* try to export */
	xml = xb_silo_export (silo, XB_NODE_EXPORT_FLAG_NONE, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
	g_assert_null (xml);
	g_clear_error (&error);

	/* try to query empty silo */
	results = xb_silo_query (silo, "components/component", 0, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
	g_assert_null (results);
	g_clear_error (&error);

	/* load blob */
	g_assert_nonnull (bytes);
	ret = xb_silo_load_from_bytes (silo2, bytes, 0, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
}

static void
xb_xpath_node_func (void)
{
	g_autoptr(GError) error = NULL;
	g_autoptr(GPtrArray) results = NULL;
	g_autoptr(XbNode) n = NULL;
	g_autoptr(XbSilo) silo = NULL;
	const gchar *xml =
	"<components origin=\"lvfs\">\n"
	"  <component type=\"desktop\">\n"
	"    <id>gimp.desktop</id>\n"
	"    <id>org.gnome.Gimp.desktop</id>\n"
	"  </component>\n"
	"  <component type=\"firmware\">\n"
	"    <id>org.hughski.ColorHug2.firmware</id>\n"
	"  </component>\n"
	"</components>\n";

	/* import from XML */
	silo = xb_silo_new_from_xml (xml, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* get node */
	n = xb_silo_query_first (silo, "components/component", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_attr (n, "type"), ==, "desktop");

	/* query with text opcodes */
	results = xb_node_query (n, "id", 0, &error);
	g_assert_no_error (error);
	g_assert_nonnull (results);
	g_assert_cmpint (results->len, ==, 2);
}

static void
xb_xpath_helpers_func (void)
{
	const gchar *tmp;
	guint64 val;
	g_autoptr(GError) error = NULL;
	g_autoptr(XbNode) n = NULL;
	g_autoptr(XbSilo) silo = NULL;

	/* import from XML */
	silo = xb_silo_new_from_xml ("<release><checksum size=\"123\">456</checksum></release>", &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* as char */
	n = xb_silo_get_root (silo);
	g_assert_nonnull (n);
	tmp = xb_node_query_text (n, "checksum", &error);
	g_assert_no_error (error);
	g_assert_cmpstr (tmp, ==, "456");
	tmp = xb_node_query_attr (n, "checksum", "size", &error);
	g_assert_no_error (error);
	g_assert_cmpstr (tmp, ==, "123");

	/* as uint64 */
	val = xb_node_query_text_as_uint (n, "checksum", &error);
	g_assert_no_error (error);
	g_assert_cmpint (val, ==, 456);
	val = xb_node_query_attr_as_uint (n, "checksum", "size", &error);
	g_assert_no_error (error);
	g_assert_cmpint (val, ==, 123);
}

static void
xb_xpath_query_func (void)
{
	g_autoptr(GError) error = NULL;
	g_autoptr(XbNode) n = NULL;
	g_autoptr(XbSilo) silo = NULL;
	const gchar *xml =
	"<components>\n"
	"  <component>\n"
	"    <id>n/a</id>\n"
	"  </component>\n"
	"</components>\n";

	/* import from XML */
	silo = xb_silo_new_from_xml (xml, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* query with slash */
	n = xb_silo_query_first (silo, "components/component/id[text()='n\\/a']", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "n/a");
}

static void
xb_xpath_func (void)
{
	XbNode *n;
	XbNode *n2;
	g_autofree gchar *str = NULL;
	g_autofree gchar *xml_sub1 = NULL;
	g_autofree gchar *xml_sub2 = NULL;
	g_autofree gchar *xml_sub3 = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(GPtrArray) results = NULL;
	g_autoptr(XbNode) n3 = NULL;
	g_autoptr(XbSilo) silo = NULL;
	const gchar *xml =
	"<components origin=\"lvfs\">\n"
	"  <header>\n"
	"    <csum type=\"sha1\">dead</csum>\n"
	"  </header>\n"
	"  <component type=\"desktop\">\n"
	"    <id>gimp.desktop</id>\n"
	"    <id>org.gnome.Gimp.desktop</id>\n"
	"  </component>\n"
	"  <component type=\"firmware\">\n"
	"    <id>org.hughski.ColorHug2.firmware</id>\n"
	"  </component>\n"
	"</components>\n";

	/* import from XML */
	silo = xb_silo_new_from_xml (xml, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* set up debugging */
	xb_machine_set_debug_flags (xb_silo_get_machine (silo),
				    XB_MACHINE_DEBUG_FLAG_SHOW_STACK);

	/* dump to screen */
	str = xb_silo_to_string (silo, &error);
	g_assert_no_error (error);
	g_assert_nonnull (str);
	g_debug ("\n%s", str);

	/* query that doesn't find anything */
	n = xb_silo_query_first (silo, "dave", &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
	g_assert_null (n);
	g_clear_error (&error);
	g_clear_object (&n);

	n = xb_silo_query_first (silo, "dave/dave", &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
	g_assert_null (n);
	g_clear_error (&error);
	g_clear_object (&n);

	n = xb_silo_query_first (silo, "components/dave", &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
	g_assert_null (n);
	g_clear_error (&error);
	g_clear_object (&n);

	n = xb_silo_query_first (silo, "components/component[@type='dave']/id", &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
	g_assert_null (n);
	g_clear_error (&error);
	g_clear_object (&n);

	n = xb_silo_query_first (silo, "components/component[@percentage>=90]", &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
	g_assert_null (n);
	g_clear_error (&error);
	g_clear_object (&n);

	n = xb_silo_query_first (silo, "components/component/id[text()='dave']", &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
	g_assert_null (n);
	g_clear_error (&error);
	g_clear_object (&n);

	/* query with attr opcodes */
	n = xb_silo_query_first (silo, "components/component[@type='firmware']/id", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "org.hughski.ColorHug2.firmware");
	g_clear_object (&n);

	/* query with attr opcodes */
	n = xb_silo_query_first (silo, "components/component[@type!='firmware']/id", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "gimp.desktop");
	g_clear_object (&n);

	/* query with attr opcodes with quotes */
	n = xb_silo_query_first (silo, "components/component[@type='firmware']/id", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "org.hughski.ColorHug2.firmware");
	g_clear_object (&n);

	/* query with position */
	n = xb_silo_query_first (silo, "components/component[2]/id", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "org.hughski.ColorHug2.firmware");
	g_clear_object (&n);

	/* last() with position */
	n = xb_silo_query_first (silo, "components/component[last()]/id", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "org.hughski.ColorHug2.firmware");
	g_clear_object (&n);

	/* query with attr opcodes that exists */
	n = xb_silo_query_first (silo, "components/component[@type]/id", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "gimp.desktop");
	g_clear_object (&n);

	/* query with text opcodes */
	n = xb_silo_query_first (silo, "components/header/csum[text()='dead']", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_attr (n, "type"), ==, "sha1");
	g_clear_object (&n);

	/* query with search */
	n = xb_silo_query_first (silo, "components/component/id[text()~='gimp']", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "gimp.desktop");
	g_clear_object (&n);

	/* query with backtrack */
	g_debug ("\n%s", xml);
	n = xb_silo_query_first (silo, "components/component[@type='firmware']/id[text()='org.hughski.ColorHug2.firmware']", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "org.hughski.ColorHug2.firmware");
	g_clear_object (&n);

	/* query with nesting */
	g_debug ("\n%s", xml);
	n = xb_silo_query_first (silo, "components/component/id[text()=lower-case('GIMP.DESKTOP')]", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "gimp.desktop");
	g_clear_object (&n);

	/* query for multiple results */
	results = xb_silo_query (silo, "components/component/id", 5, &error);
	g_assert_no_error (error);
	g_assert_nonnull (results);
	g_assert_cmpint (results->len, ==, 3);
	n2 = g_ptr_array_index (results, 2);
	g_assert_cmpstr (xb_node_get_text (n2), ==, "org.hughski.ColorHug2.firmware");

	/* subtree export */
	xml_sub1 = xb_node_export (n2, XB_NODE_EXPORT_FLAG_NONE, &error);
	g_assert_no_error (error);
	g_assert_nonnull (xml_sub1);
	g_assert_cmpstr (xml_sub1, ==, "<id>org.hughski.ColorHug2.firmware</id>");

	/* parent of subtree */
	n3 = xb_node_get_parent (n2);
	g_assert (n3 != NULL);
	xml_sub2 = xb_node_export (n3, XB_NODE_EXPORT_FLAG_NONE, &error);
	g_assert_no_error (error);
	g_assert_nonnull (xml_sub2);
	g_assert_cmpstr (xml_sub2, ==, "<component type=\"firmware\"><id>org.hughski.ColorHug2.firmware</id></component>");

	/* only children of parent */
	xml_sub3 = xb_node_export (n3, XB_NODE_EXPORT_FLAG_ONLY_CHILDREN, &error);
	g_assert_no_error (error);
	g_assert_nonnull (xml_sub3);
	g_assert_cmpstr (xml_sub3, ==, "<id>org.hughski.ColorHug2.firmware</id>");
}

static void
xb_builder_native_lang_func (void)
{
	gboolean ret;
	g_autoptr(GError) error = NULL;
	g_autofree gchar *str = NULL;
	g_autoptr(XbBuilder) builder = xb_builder_new ();
	g_autoptr(XbSilo) silo = NULL;
	const gchar *xml =
	"<components>\n"
	"  <component>\n"
	"    <p xml:lang=\"de_DE\">Wilcommen</p>\n"
	"    <p>Hello</p>\n"
	"    <p xml:lang=\"fr\">Salut</p>\n"
	"  </component>\n"
	"</components>\n";

	/* import from XML */
	ret = xb_builder_import_xml (builder, xml, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	xb_builder_add_locale (builder, "fr_FR");
	xb_builder_add_locale (builder, "fr");
	xb_builder_add_locale (builder, "C");
	silo = xb_builder_compile (builder, XB_BUILDER_COMPILE_FLAG_SINGLE_LANG, NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* test we removed other languages */
	str = xb_silo_to_string (silo, &error);
	g_assert_no_error (error);
	g_assert_nonnull (str);
	g_debug ("\n%s", str);
	g_assert_null (g_strstr_len (str, -1, "Wilcommen"));
	g_assert_null (g_strstr_len (str, -1, "Hello"));
	g_assert_nonnull (g_strstr_len (str, -1, "Salut"));
}

static void
xb_builder_native_lang2_func (void)
{
	gboolean ret;
	g_autoptr(GError) error = NULL;
	g_autofree gchar *str = NULL;
	g_autoptr(XbBuilder) builder = xb_builder_new ();
	g_autoptr(XbSilo) silo = NULL;
	const gchar *xml =
	"<components>\n"
	"  <component>\n"
	"    <description xml:lang=\"de_DE\"><p>Wilcommen</p></description>\n"
	"    <description><p>Hello</p></description>\n"
	"    <description xml:lang=\"fr\"><p>Salut</p></description>\n"
	"  </component>\n"
	"</components>\n";

	/* import from XML */
	ret = xb_builder_import_xml (builder, xml, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	xb_builder_add_locale (builder, "fr_FR");
	xb_builder_add_locale (builder, "fr");
	xb_builder_add_locale (builder, "C");
	silo = xb_builder_compile (builder, XB_BUILDER_COMPILE_FLAG_SINGLE_LANG, NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* test we removed other languages */
	str = xb_silo_to_string (silo, &error);
	g_assert_no_error (error);
	g_assert_nonnull (str);
	g_assert_null (g_strstr_len (str, -1, "Wilcommen"));
	g_assert_null (g_strstr_len (str, -1, "Hello"));
	g_assert_nonnull (g_strstr_len (str, -1, "Salut"));
	g_debug ("\n%s", str);
}

static void
xb_xpath_parent_func (void)
{
	XbNode *n;
	gboolean ret;
	g_autoptr(GError) error = NULL;
	g_autoptr(XbBuilder) builder = xb_builder_new ();
	g_autoptr(XbSilo) silo = NULL;
	const gchar *xml =
	"<components origin=\"lvfs\">\n"
	"  <header>\n"
	"    <csum type=\"sha1\">dead</csum>\n"
	"  </header>\n"
	"  <component type=\"desktop\">\n"
	"    <id>gimp.desktop</id>\n"
	"    <id>org.gnome.Gimp.desktop</id>\n"
	"  </component>\n"
	"  <component type=\"firmware\">\n"
	"    <id>org.hughski.ColorHug2.firmware</id>\n"
	"    <pkgname>colorhug-client</pkgname>\n"
	"    <description xml:lang=\"de_DE\"><p>Wilcommen!</p></description>\n"
	"    <description><p>hello!</p></description>\n"
	"    <description xml:lang=\"fr_FR\"><p>Bonjour!</p></description>\n"
	"    <project_license>GPL-2.0</project_license>\n"
	"  </component>\n"
	"</components>\n";

	/* import from XML */
	ret = xb_builder_import_xml (builder, xml, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	xb_builder_add_locale (builder, "C");
	silo = xb_builder_compile (builder, XB_BUILDER_COMPILE_FLAG_NATIVE_LANGS, NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* get node, no parent */
	n = xb_silo_query_first (silo, "components/component[@type='firmware']/id", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "org.hughski.ColorHug2.firmware");
	g_assert_cmpstr (xb_node_get_element (n), ==, "id");
	g_clear_object (&n);

	/* get node, one parent */
	n = xb_silo_query_first (silo, "components/component[@type='firmware']/id/..", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_element (n), ==, "component");
	g_clear_object (&n);

	/* get node, multiple parents */
	n = xb_silo_query_first (silo, "components/component[@type='firmware']/id/../..", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_element (n), ==, "components");
	g_clear_object (&n);

	/* descend, ascend, descend */
	n = xb_silo_query_first (silo, "components/component[@type='firmware']/pkgname/../project_license", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "GPL-2.0");
	g_clear_object (&n);

	/* descend, ascend, descend */
	n = xb_silo_query_first (silo, "components/component/pkgname[text()~='colorhug']/../id", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "org.hughski.ColorHug2.firmware");
	g_clear_object (&n);

	/* get node, too many parents */
	n = xb_silo_query_first (silo, "components/component[@type='firmware']/id/../../..", &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
	g_assert_null (n);
	g_clear_error (&error);

	/* can't go lower than root */
	n = xb_silo_query_first (silo, "..", &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
	g_assert_null (n);
	g_clear_error (&error);

	/* fuzzy substring match */
	n = xb_silo_query_first (silo, "components/component/pkgname[text()~='colorhug']", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "colorhug-client");
	g_clear_object (&n);

	/* fuzzy substring match */
	n = xb_silo_query_first (silo, "components/component[@type~='ware']/pkgname", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "colorhug-client");
	g_clear_object (&n);
}

static void
xb_xpath_glob_func (void)
{
	g_autofree gchar *xml2 = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(XbNode) n = NULL;
	g_autoptr(XbSilo) silo = NULL;
	const gchar *xml =
	"<components origin=\"lvfs\">\n"
	"  <component type=\"desktop\">\n"
	"    <id>gimp.desktop</id>\n"
	"    <id>org.gnome.GIMP.desktop</id>\n"
	"  </component>\n"
	"</components>\n";

	/* import from XML */
	silo = xb_silo_new_from_xml (xml, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* get node, no parent */
	n = xb_silo_query_first (silo, "components/component[@type='desktop']/*", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_element (n), ==, "id");

	/* export this one node */
	xml2 = xb_node_export (n, XB_NODE_EXPORT_FLAG_NONE, &error);
	g_assert_no_error (error);
	g_assert_cmpstr (xml2, ==, "<id>gimp.desktop</id>");
}

static void
xb_builder_multiple_roots_func (void)
{
	gboolean ret;
	g_autofree gchar *str = NULL;
	g_autofree gchar *xml_new = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(GPtrArray) results = NULL;
	g_autoptr(XbBuilder) builder = NULL;
	g_autoptr(XbSilo) silo = NULL;

	/* import from XML */
	builder = xb_builder_new ();
	ret = xb_builder_import_xml (builder, "<tag>value</tag>", &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	ret = xb_builder_import_xml (builder, "<tag>value2</tag>", &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	silo = xb_builder_compile (builder, XB_BUILDER_COMPILE_FLAG_NONE, NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* convert back to XML */
	str = xb_silo_to_string (silo, &error);
	g_assert_no_error (error);
	g_assert_nonnull (str);
	g_debug ("\n%s", str);
	xml_new = xb_silo_export (silo, XB_NODE_EXPORT_FLAG_INCLUDE_SIBLINGS, &error);
	g_assert_no_error (error);
	g_assert_nonnull (xml_new);
	g_print ("%s", xml_new);
	g_assert_cmpstr ("<tag>value</tag><tag>value2</tag>", ==, xml_new);

	/* query for multiple results */
	results = xb_silo_query (silo, "tag", 5, &error);
	g_assert_no_error (error);
	g_assert_nonnull (results);
	g_assert_cmpint (results->len, ==, 2);
}

static void
xb_builder_node_func (void)
{
	g_autofree gchar *xml = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(XbBuilder) builder = xb_builder_new ();
	g_autoptr(XbBuilderNode) component = NULL;
	g_autoptr(XbBuilderNode) components = NULL;
	g_autoptr(XbBuilderNode) id = NULL;
	g_autoptr(XbSilo) silo = NULL;

	/* create a simple document */
	components = xb_builder_node_insert (NULL, "components",
					     "origin", "lvfs",
					     NULL);
	component = xb_builder_node_insert (components, "component", NULL);
	xb_builder_node_set_attr (component, "type", "desktop");
	id = xb_builder_node_new ("id");
	xb_builder_node_add_child (component, id);
	xb_builder_node_set_text (id, "gimp.desktop", -1);
	xb_builder_node_insert_text (component, "icon", "dave", "type", "stock", NULL);

	/* import the doc */
	xb_builder_import_node (builder, components);
	silo = xb_builder_compile (builder, XB_BUILDER_COMPILE_FLAG_NONE, NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* check the XML */
	xml = xb_silo_export (silo, XB_NODE_EXPORT_FLAG_INCLUDE_SIBLINGS, &error);
	g_assert_no_error (error);
	g_assert_nonnull (xml);
	g_print ("%s", xml);
	g_assert_cmpstr ("<components origin=\"lvfs\">"
			 "<component type=\"desktop\">"
			 "<id>gimp.desktop</id>"
			 "<icon type=\"stock\">dave</icon>"
			 "</component>"
			 "</components>", ==, xml);
}

static void
xb_builder_node_info_func (void)
{
	const gchar *fn = "/tmp/xb-self-test.xml";
	gboolean ret;
	g_autofree gchar *xml = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(XbBuilder) builder = xb_builder_new ();
	g_autoptr(XbNode) n = NULL;
	g_autoptr(XbBuilderNode) info1 = NULL;
	g_autoptr(XbBuilderNode) info2 = NULL;
	g_autoptr(XbSilo) silo = NULL;
	g_autoptr(GFile) file = NULL;

	/* create a simple document with some info */
	ret = g_file_set_contents (fn, "<component><id type=\"desktop\">dave</id></component>", -1, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	info1 = xb_builder_node_insert (NULL, "info", NULL);
	xb_builder_node_insert_text (info1, "scope", "user", NULL);
	info2 = xb_builder_node_insert (NULL, "info", NULL);
	xb_builder_node_insert_text (info2, "scope", "system", NULL);

	/* import the doc */
	file = g_file_new_for_path (fn);
	ret = xb_builder_import_file (builder, file, info1, NULL, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	ret = xb_builder_import_file (builder, file, info2, NULL, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	silo = xb_builder_compile (builder, XB_BUILDER_COMPILE_FLAG_NONE, NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);

	/* get info */
	n = xb_silo_query_first (silo, "component/id[text()='dave']/../info/scope", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_assert_cmpstr (xb_node_get_text (n), ==, "user");

	/* check the XML */
	xml = xb_silo_export (silo, XB_NODE_EXPORT_FLAG_INCLUDE_SIBLINGS, &error);
	g_assert_no_error (error);
	g_assert_nonnull (xml);
	g_assert_cmpstr ("<component>"
			 "<id type=\"desktop\">dave</id>"
			 "<info>"
			 "<scope>user</scope>"
			 "</info>"
			 "</component>"
			 "<component>"
			 "<id type=\"desktop\">dave</id>"
			 "<info>"
			 "<scope>system</scope>"
			 "</info>"
			 "</component>", ==, xml);
}

static void
xb_speed_func (void)
{
	XbNode *n;
	const gchar *fn = "/tmp/test.xmlb";
	gboolean ret;
	guint n_components = 10000;
	g_autofree gchar *xpath1 = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(GFile) file = NULL;
	g_autoptr(GPtrArray) results = NULL;
	g_autoptr(GString) xml = g_string_new (NULL);
	g_autoptr(GTimer) timer = g_timer_new ();
	g_autoptr(XbSilo) silo = NULL;

	/* create a huge document */
	g_string_append (xml, "<components>");
	for (guint i = 0; i < n_components; i++) {
		g_string_append (xml, "<component>");
		g_string_append_printf (xml, "  <id>%06u.firmware</id>", i);
		g_string_append (xml, "  <name>ColorHug2</name>");
		g_string_append (xml, "  <summary>Firmware</summary>");
		g_string_append (xml, "  <description><p>New features!</p></description>");
		g_string_append (xml, "  <provides>");
		g_string_append (xml, "    <firmware type=\"flashed\">2082b5e0</firmware>");
		g_string_append (xml, "  </provides>");
		g_string_append (xml, "  <requires>");
		g_string_append (xml, "    <id compare=\"ge\" version=\"0.8.0\">fwupd</id>");
		g_string_append (xml, "    <firmware compare=\"eq\" version=\"2.0.99\"/>");
		g_string_append (xml, "  </requires>");
		g_string_append (xml, "  <url type=\"homepage\">http://com/</url>");
		g_string_append (xml, "  <metadata_license>CC0-1.0</metadata_license>");
		g_string_append (xml, "  <project_license>GPL-2.0+</project_license>");
		g_string_append (xml, "  <updatecontact>richard</updatecontact>");
		g_string_append (xml, "  <developer_name>Hughski</developer_name>");
		g_string_append (xml, "  <releases>");
		g_string_append (xml, "    <release urgency=\"medium\" version=\"2.0.3\" timestamp=\"1429362707\">");
		g_string_append (xml, "      <description><p>stable:</p><ul><li>Quicker</li></ul></description>");
		g_string_append (xml, "    </release>");
		g_string_append (xml, "  </releases>");
		g_string_append (xml, "</component>");
	}
	g_string_append (xml, "</components>");

	/* import from XML */
	silo = xb_silo_new_from_xml (xml->str, &error);
	g_assert_no_error (error);
	g_assert_nonnull (silo);
	file = g_file_new_for_path (fn);
	ret = xb_silo_save_to_file (silo, file, NULL, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	g_clear_object (&silo);
	g_print ("import+save: %.3fms\n", g_timer_elapsed (timer, NULL) * 1000);
	g_timer_reset (timer);

	/* load from file */
	silo = xb_silo_new ();
	ret = xb_silo_load_from_file (silo, file, XB_SILO_LOAD_FLAG_NONE, NULL, &error);
	g_assert_no_error (error);
	g_assert_true (ret);
	g_print ("mmap load: %.3fms\n", g_timer_elapsed (timer, NULL) * 1000);
	g_timer_reset (timer);

	/* query best case */
	n = xb_silo_query_first (silo, "components/component/id[text()='000000.firmware']", &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_print ("query[first]: %.3fms\n", g_timer_elapsed (timer, NULL) * 1000);
	g_timer_reset (timer);
	g_clear_object (&n);

	/* query worst case */
	xpath1 = g_strdup_printf ("components/component/id[text()='%06u.firmware']", n_components - 1);
	n = xb_silo_query_first (silo, xpath1, &error);
	g_assert_no_error (error);
	g_assert_nonnull (n);
	g_print ("query[last]: %.3fms\n", g_timer_elapsed (timer, NULL) * 1000);
	g_timer_reset (timer);
	g_clear_object (&n);

	/* query all components */
	results = xb_silo_query (silo, "components/component", 0, &error);
	g_assert_no_error (error);
	g_assert_nonnull (results);
	g_assert_cmpint (results->len, ==, n_components);
	g_print ("query[all]: %.3fms\n", g_timer_elapsed (timer, NULL) * 1000);
	g_timer_reset (timer);

	/* factorial search */
	for (guint i = 0; i < n_components; i += 20) {
		g_autofree gchar *xpath2 = NULL;
		xpath2 = g_strdup_printf ("components/component/id[text()='%06u.firmware']", i);
		n = xb_silo_query_first (silo, xpath2, &error);
		g_assert_no_error (error);
		g_assert_nonnull (n);
		g_clear_object (&n);
	}
	g_print ("query[x%u]: %.3fms\n", n_components, g_timer_elapsed (timer, NULL) * 1000);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	/* only critical and error are fatal */
	g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);
	g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

	/* tests go here */
	g_test_add_func ("/libxmlb/opcodes", xb_predicate_func);
	g_test_add_func ("/libxmlb/builder", xb_builder_func);
	g_test_add_func ("/libxmlb/builder{native-lang}", xb_builder_native_lang_func);
	g_test_add_func ("/libxmlb/builder{native-lang-nested}", xb_builder_native_lang2_func);
	g_test_add_func ("/libxmlb/builder{empty}", xb_builder_empty_func);
	g_test_add_func ("/libxmlb/builder{ensure}", xb_builder_ensure_func);
	g_test_add_func ("/libxmlb/builder{node-vfunc}", xb_builder_node_vfunc_func);
	g_test_add_func ("/libxmlb/builder-node", xb_builder_node_func);
	g_test_add_func ("/libxmlb/builder-node{info}", xb_builder_node_info_func);
	g_test_add_func ("/libxmlb/xpath", xb_xpath_func);
	g_test_add_func ("/libxmlb/xpath-query", xb_xpath_query_func);
	g_test_add_func ("/libxmlb/xpath{helpers}", xb_xpath_helpers_func);
	g_test_add_func ("/libxmlb/xpath-parent", xb_xpath_parent_func);
	g_test_add_func ("/libxmlb/xpath-glob", xb_xpath_glob_func);
	g_test_add_func ("/libxmlb/xpath-node", xb_xpath_node_func);
	g_test_add_func ("/libxmlb/multiple-roots", xb_builder_multiple_roots_func);
	g_test_add_func ("/libxmlb/speed", xb_speed_func);
	return g_test_run ();
}
