#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <glib.h>

#include <libcam/cam.h>

typedef struct _state_t {
    GMainLoop *mainloop;
    char filename[4096];
    int write_count;
} state_t;

static void
print_usage_and_inputs (const char *progname, CamUnitManager *manager)
{
    fprintf (stderr, "usage: %s <input_id>\n\n", progname);
    fprintf (stderr, "Available inputs:\n\n"); 
    GList *udlist = cam_unit_manager_list_package (manager, "input", TRUE);
    for (GList *uditer=udlist; uditer; uditer=uditer->next) {
        CamUnitDescription *udesc = CAM_UNIT_DESCRIPTION(uditer->data);
        printf("  %s  (%s)\n", udesc->unit_id, udesc->name);
    }
    g_list_free(udlist);
}

static void
on_frame_ready (CamUnitChain *chain, CamUnit *unit, 
        const CamFrameBuffer *buf, void *user_data)
{
    state_t *s = user_data;
    if (s->write_count) return;
    FILE *fp = fopen (s->filename, "wb");
    if (!fp) {
        perror ("fopen");
        g_main_loop_quit (s->mainloop);
        return;
    }

    // write the image to disk as a PPM
    const CamUnitFormat *fmt = cam_unit_get_output_format (unit);
    fprintf(fp, "P6 %d %d %d\n", fmt->width, fmt->height, 255);
    for (int i=0; i<fmt->height; i++){
        int count = fwrite(buf->data + i*fmt->row_stride, fmt->width*3, 1, fp);
        if (1 != count) {
            perror ("fwrite");
        }
    }
    fclose (fp);
    printf ("wrote %s\n", s->filename);
    s->write_count++;
    g_main_loop_quit (s->mainloop);
}

int main(int argc, char **argv)
{
    g_type_init();

    state_t s;
    memset (&s, 0, sizeof (s));
    sprintf (s.filename, "snapshot-output.ppm");

    // create the GLib mainloop
    s.mainloop = g_main_loop_new (NULL, FALSE);

    // create the image processing chain
    CamUnitChain * chain = cam_unit_chain_new ();

    // abort if no input unit was specified
    if (argc < 2) {
        print_usage_and_inputs (argv[0], chain->manager);
        goto failed;
    }
    const char *input_id = argv[1];

    // instantiate the input unit
    if (! cam_unit_chain_add_unit_by_id (chain, input_id)) {
        fprintf (stderr, "Oh no!  Couldn't create input unit [%s].\n\n", 
                input_id);
        print_usage_and_inputs (argv[0], chain->manager);
        goto failed;
    }

    // create a unit to convert the input data to 8-bit RGB
    CamUnit *to_rgb8 = cam_unit_chain_add_unit_by_id (chain, "convert.to_rgb8");

    // start the image processing chain
    cam_unit_chain_set_desired_status (chain, CAM_UNIT_STATUS_READY);

    // did everything start up correctly?
    CamUnit *faulty_unit = cam_unit_chain_check_status_all_units (chain, 
                CAM_UNIT_STATUS_READY);
    if (faulty_unit) {
        fprintf (stderr, "Unit [%s] is not streaming, aborting...\n",
                cam_unit_get_name (faulty_unit));
        goto failed;
    }

    // attach the chain to the glib event loop.
    cam_unit_chain_attach_glib (chain, 1000, NULL);

    // subscribe to be notified when an image has made its way through the
    // chain
    g_signal_connect (G_OBJECT (chain), "frame-ready",
            G_CALLBACK (on_frame_ready), &s);

    // run 
    g_main_loop_run (s.mainloop);

    // cleanup
    g_main_loop_unref (s.mainloop);
    cam_unit_chain_set_desired_status (chain, CAM_UNIT_STATUS_IDLE);
    g_object_unref (chain);
    return 0;

failed:
    g_main_loop_unref (s.mainloop);
    cam_unit_chain_set_desired_status (chain, CAM_UNIT_STATUS_IDLE);
    g_object_unref (chain);
    return 1;
}
