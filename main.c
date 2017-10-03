//
// Created by joe on 5/3/17.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <jack/jack.h>
#include <jack/session.h>
#include <math.h>
#include <gtk/gtk.h>
jack_port_t *input_portL , *input_portR;
jack_port_t *output_portL , *output_portR;
jack_client_t *client;

float peakL = 0.0f;
float peakR = 0.0f;
GtkWidget *win=0;
#define SPECWIDTH 48
#define SPECHEIGHT 227
#define BANDS 2

typedef struct {
    uint8_t rgbRed,rgbGreen,rgbBlue;
} RGB;

GtkWidget *speci;
GdkPixbuf *specpb;
RGB palette[SPECHEIGHT];

float barFallDelay = 6.0f;
float peakFallDelay = 45.0f;

double minDBValue = -40;
double maxDBValue = 3;
double dbScale;

double PeakMeterLHeight = 0;
double PeakMeterRHeight = 0;
double PeakMeterLHeightPeaks = 0;
double PeakMeterRHeightPeaks = 0;

double peakDataLeft = -30;
double peakDataRight = -30;
double peakDataLeftPeaks = -30;
double peakDataRightPeaks = -30;
int peakLeftTrigger , peakRightTrigger = 0;

float read_peakL() {
    float tmpL = peakL;
    peakL = 0.0f;
    return tmpL;
}
float read_peakR() {
    float tmpR = peakR;
    peakR = 0.0f;
    return tmpR;
}
int process (jack_nframes_t nframes, void *arg) {
    jack_default_audio_sample_t *inL, *inR;

    inL = jack_port_get_buffer (input_portL, nframes);
    inR = jack_port_get_buffer (input_portR, nframes);

    for (unsigned int i = 0; i < nframes; i++) {
        const float sL = (const float) fabs(inL[i]);
        const float sR = (const float) fabs(inR[i]);
        if (sL > peakL) {
            peakL = sL;
        }
        if (sR > peakR) {
            peakR = sR;
        }
    }
    return 0;
}

void jack_shutdown (void *arg) {
    exit (1);
}
void WindowDestroy(GtkObject *obj, gpointer data) {
    jack_client_close (client);
    gtk_main_quit();
}
void Init_Gtk(int argc, char *argv[]){
    gtk_init(&argc,&argv);

    win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_keep_above(GTK_WINDOW(win),TRUE);
    gtk_window_set_position(GTK_WINDOW(win),GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(win),FALSE);
    gtk_window_set_title(GTK_WINDOW(win),"PW_PeakMeter");
    gtk_window_set_type_hint(GTK_WINDOW(win),GDK_WINDOW_TYPE_HINT_MENU);
    g_signal_connect(GTK_WINDOW(win),"destroy",GTK_SIGNAL_FUNC(WindowDestroy),NULL);

    GtkWidget *ebox=gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(win),ebox);

    GtkWidget *box=gtk_vbox_new(FALSE,2);
    gtk_container_add(GTK_CONTAINER(ebox),box);

    specpb=gdk_pixbuf_new(GDK_COLORSPACE_RGB,FALSE,8,SPECWIDTH,SPECHEIGHT);
    speci=gtk_image_new_from_pixbuf(specpb);
    gtk_container_add(GTK_CONTAINER(box),speci);

    {
        RGB *pal=palette;
        memset(palette,0,sizeof(palette));
        for (int a=0;a<SPECHEIGHT;a++) {
            pal[a].rgbGreen= (uint8_t) -a;
            pal[a].rgbRed= (uint8_t) ((uint8_t) a / 2);
        }
    }
     dbScale = (maxDBValue - minDBValue);
}

gboolean UpdateSpectrum(gpointer data) {

    RGB *specbuf=(RGB*)gdk_pixbuf_get_pixels(specpb);
    int y,y1,yR,y1R;
    int yP,y1P,yRP,y1RP;
    double peak1L;
    double peak1R;

    memset(specbuf,0,SPECWIDTH*SPECHEIGHT*sizeof(*specbuf));

    peak1L = 20*log10(read_peakL());
    peak1R = 20*log10(read_peakR());

    if (peakDataLeft < peak1L) {
        peakDataLeft = peak1L;
    }else{
        peakDataLeft = ((peak1L + (barFallDelay * peakDataLeft)) / ((barFallDelay + 1)));
    }
    if (peakDataRight < peak1R) {
        peakDataRight = peak1R;
    }else{
        peakDataRight = ((peak1R + (barFallDelay * peakDataRight)) / ((barFallDelay + 1)));
    }

    peakLeftTrigger = peakLeftTrigger - 1;
    if (peakDataLeftPeaks < peak1L) {
        peakDataLeftPeaks = peak1L;
        peakLeftTrigger = 100;
    }else{
        if(peakLeftTrigger < 1) {
            peakDataLeftPeaks = ((peak1L + (peakFallDelay * peakDataLeftPeaks)) / ((peakFallDelay + 1)));
        }
    }

    peakRightTrigger = peakRightTrigger - 1;
    if (peakDataRightPeaks < peak1R) {
        peakDataRightPeaks = peak1R;
        peakRightTrigger = 100;
    }else{
        if (peakRightTrigger < 1) {
            peakDataRightPeaks = ((peak1R + (peakFallDelay * peakDataRightPeaks)) / ((peakFallDelay + 1)));
        }
    }

    PeakMeterRHeight= ((peakDataRight - minDBValue) / dbScale) * (SPECHEIGHT);
    if (PeakMeterRHeight <= 0){
        PeakMeterRHeight = 1;
    }
    PeakMeterLHeight= ((peakDataLeft - minDBValue) / dbScale) * (SPECHEIGHT);
    if (PeakMeterLHeight <= 0){
        PeakMeterLHeight = 1;
    }

    PeakMeterLHeightPeaks= ((peakDataLeftPeaks - minDBValue) / dbScale) * (SPECHEIGHT);
    if (PeakMeterLHeightPeaks <= 0){
        PeakMeterLHeightPeaks = 1;
    }
    PeakMeterRHeightPeaks= ((peakDataRightPeaks - minDBValue) / dbScale) * (SPECHEIGHT);
    if (PeakMeterRHeightPeaks <= 0){
        PeakMeterRHeightPeaks = 1;
    }

    y= (int) (PeakMeterLHeight);
    if (y>PeakMeterLHeight) y= (int) PeakMeterLHeight;
        while (--y>=0)
            for (y1=0;y1<SPECWIDTH/BANDS-2;y1++)
                specbuf[(SPECHEIGHT-1-y)*SPECWIDTH+0*(SPECWIDTH/BANDS)+y1]=palette[y+1];

    yR= (int) PeakMeterRHeight;
    if (yR>PeakMeterRHeight) yR= (int) PeakMeterRHeight;
        while (--yR>=0)
            for (y1R=0;y1R<SPECWIDTH/BANDS-2;y1R++)
                specbuf[(SPECHEIGHT-1-yR)*SPECWIDTH+1*(SPECWIDTH/BANDS)+y1R]=palette[yR+1];

    yP= (int) PeakMeterLHeightPeaks;
    if (yP>PeakMeterLHeightPeaks) yP= (int) PeakMeterLHeightPeaks;
        for (y1P=0;y1P<SPECWIDTH/BANDS-2;y1P++)
            specbuf[(SPECHEIGHT-yP)*SPECWIDTH+0*(SPECWIDTH/BANDS)+y1P]=palette[yP+1];

    yRP= (int) PeakMeterRHeightPeaks;
    if (yRP>PeakMeterRHeightPeaks) yRP= (int) PeakMeterRHeightPeaks;
        for (y1RP=0;y1RP<SPECWIDTH/BANDS-2;y1RP++)
            specbuf[(SPECHEIGHT-yRP)*SPECWIDTH+1*(SPECWIDTH/BANDS)+y1RP]=palette[yRP+1];

    gtk_image_set_from_pixbuf(GTK_IMAGE(speci),specpb);

    return TRUE;
}
int main (int argc, char *argv[]) {
    const char **ports;
    const char *client_name = "PW_PeakMeter";
    jack_status_t status;

    Init_Gtk(argc,argv);

    client = jack_client_open (client_name, JackNullOption, &status );

    if (client == NULL) {
        fprintf (stderr, "jack_client_open() failed, "
                "status = 0x%2.0x\n", status);
        if (status & JackServerFailed) {
            fprintf (stderr, "Unable to connect to JACK server\n");
        }
        exit (1);
    }
    if (status & JackServerStarted) {
        fprintf (stderr, "JACK server started\n");
    }
    if (status & JackNameNotUnique) {
        client_name = jack_get_client_name(client);
        fprintf (stderr, "unique name `%s' assigned\n", client_name);
    }

    jack_set_process_callback (client, process, 0);

    jack_on_shutdown (client, jack_shutdown, 0);

    printf ("engine sample rate: %" PRIu32 "\n", jack_get_sample_rate (client));

    input_portL = jack_port_register (client, "inL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    input_portR = jack_port_register (client, "inR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

    if ((input_portL == NULL) || (input_portR == NULL)) {
        fprintf(stderr, "no more JACK ports available\n");
        exit (1);
    }

    if (jack_activate (client)) {
        fprintf (stderr, "cannot activate client");
        exit (1);
    }

    if (argc==1) {

        ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput);
        if (ports == NULL) {
            fprintf(stderr, "no physical capture ports\n");
            exit (1);
        }

        if (jack_connect (client, ports[0], jack_port_name (input_portL))) {
            fprintf (stderr, "cannot connect input ports\n");
        }
        if (jack_connect (client, ports[1], jack_port_name (input_portR))) {
            fprintf (stderr, "cannot connect input ports\n");
        }
        free (ports);
    }

    g_timeout_add(15,UpdateSpectrum,NULL);
    gtk_widget_show_all(win);
    gtk_main();

    g_object_unref(specpb);

}