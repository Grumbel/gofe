#include <gtk/gtk.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

GtkWidget*              box;
GtkWidget*             vbox;
GtkWidget*           button;
GtkWidget*           output;
GtkWidget*           window;
GtkWidget*          optargs;
GtkWidget*        file_name;
GtkWidget*      play_button;
GtkWidget*      stop_button;
GtkWidget*     mixer_button;
GtkWidget* find_file_button;

FILE*         proc;
char*          bin;
char*        label;
char*  default_dir;
char* default_args;
char*        mixer;
gchar*    filename;
pid_t        child;

void
file_ok_sel (GtkWidget *w, GtkFileSelection *fs)
{
  if (filename) 
    g_free(filename);

  filename = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION (fs)));

  gtk_entry_set_text(GTK_ENTRY(file_name),
		     gtk_file_selection_get_filename(GTK_FILE_SELECTION (fs)));
  gtk_widget_destroy(GTK_WIDGET(fs));
}


/* another callback */
void
destroy (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}

void
start_mixer(GtkWidget* widget, gpointer data)
{
  char* argv[] = {mixer, NULL};
  
  puts("Starting Mixer");
  if (vfork() == 0) {
    if (execv(mixer, argv) == -1) {
      perror(mixer);
    }    
    return;
  }  
}

void
find_file (GtkWidget *widget, gpointer data)
{
  GtkWidget *filew;
  
  /* Create a new file selection widget */
  filew = gtk_file_selection_new ("File selection");
  
  //  gtk_signal_connect (GTK_OBJECT (filew), "destroy",
  //	      (GtkSignalFunc) destroy, &filew);

  /* Connect the ok_button to file_ok_sel function */
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button),
		      "clicked", (GtkSignalFunc) file_ok_sel, filew );
  
  /* Connect the cancel_button to destroy the widget */
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button),
			     "clicked", (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (filew));
  
  /* Lets set the filename, as if this were a save dialog, and we are giving
     a default filename */
  gtk_file_selection_set_filename (GTK_FILE_SELECTION(filew), 
				   filename);
  gtk_widget_show(filew);
}

void
stop (GtkWidget *widget, gpointer data)
{
  if (child) {
    kill(child, SIGKILL);
  }
}

void
play (GtkWidget *widget, gpointer data)
{
  char  out[102];
  char*      str;
  char* argv[25];
  int        a=0;
  int        c=0;
  int        i=0;
  
  str = g_strdup(gtk_entry_get_text (GTK_ENTRY(optargs)));

  argv[0] = g_strdup(bin);
  
  if (child) {
    kill(child, SIGKILL);
  }

  for(i=0, a=0, c=1; str[i] != '\0'; ++i) {
    if (str[i] == ' ') {
      str[i] = '\0';
      argv[c++] = str + a;
      a = i + 1;
    }
  }

  if (i>0) 
    argv[c++] = str + a;

  argv[c] = g_strdup(gtk_entry_get_text(GTK_ENTRY(file_name)));
  argv[c+1] = NULL;

  if ((child = vfork()) == 0) {
    if (execv(bin, argv) == -1) {
      perror(bin);
    }    
    return;
  }
}

gint delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  g_print ("delete event occurred\n");
  
  if (child)
    kill(child, SIGKILL);

  return (FALSE);
}

int
main (int argc, char *argv[])
{
  int c;

  /* this is called in all GTK applications.  arguments are parsed from
   * the command line and are returned to the application. */
  gtk_init (&argc, &argv);

  opterr = 0;
  while ((c = getopt (argc, argv, "b:f:l:a:d:m:")) != -1) {
    switch (c) {
    case 'b':
      bin = g_strdup(optarg);
      break;
    case 'l':
      label = g_strdup(optarg);
      break;
    case 'f':
      filename = g_strdup(optarg);
      break;
    case 'a':
      default_args = g_strdup(optarg);
      break;
    case 'd':
      default_dir = g_strdup(optarg);
      break;
    case 'm':
      mixer = g_strdup(optarg);
      break;
    case '?':
      puts("Usage: gofe [OPTION]...\n"
	   "\n"
	   "-b    set the proramm to play\n" 
	   "-f    set the default file to play\n" 
	   "-h    display this help and exit\n"
	   "-l    set the window title  \n"
	   "-m    set the mixer application\n"
	   );
      exit (EXIT_SUCCESS);
    }
  }
  
  if (!filename)
    filename = g_strdup("");

  if (!default_args)
    default_args = g_strdup("");

  if (!default_dir)
    default_dir = g_strdup("");

  if (!label)
    label = g_strdup("GOFE - GNU's Optuse Front End");
        
  if (!bin) {
    puts("Need -b agrgument!");
    exit(EXIT_FAILURE);
  }
  /* create a new window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW(window), label);

  /* when the window is given the "delete_event" signal (this is given
   * by the window manager, usually by the 'close' option, or on the
   * titlebar), we ask it to call the delete_event () function
   * as defined above.  The data passed to the callback
   * function is NULL and is ignored in the callback function. */
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (delete_event), NULL);
          
  /* here we connect the "destroy" event to a signal handler.  
           * This event occurs when we call gtk_widget_destroy() on the window,
           * or if we return 'FALSE' in the "delete_event" callback. */
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (destroy), NULL);
          
  /* sets the border width of the window. */
  gtk_container_border_width (GTK_CONTAINER (window), 10);
  
  box = gtk_hbox_new(0,0);
  vbox = gtk_vbox_new(0,0);

  file_name = gtk_entry_new();
  output = gtk_label_new(label);

  play_button = gtk_button_new_with_label("Play");
  stop_button = gtk_button_new_with_label("Stop");
  find_file_button = gtk_button_new_with_label("File");

  optargs = gtk_entry_new();

  gtk_entry_set_text (GTK_ENTRY(optargs), default_args);
  gtk_entry_set_text (GTK_ENTRY(file_name), filename);

  gtk_box_pack_start (GTK_BOX (box), play_button, 1, 1, 0);
  gtk_box_pack_start (GTK_BOX (box), stop_button, 1, 1, 0);
  gtk_box_pack_start (GTK_BOX (box), find_file_button, 1, 1, 0);
  gtk_box_pack_start (GTK_BOX (box), optargs, 1, 1, 0);

  if (mixer) {
    mixer_button = gtk_button_new_with_label("Mixer");
    gtk_box_pack_start (GTK_BOX (box), mixer_button, 1, 1, 0);
    gtk_signal_connect (GTK_OBJECT (mixer_button), "clicked",
			GTK_SIGNAL_FUNC (start_mixer), NULL);
    gtk_widget_show(mixer_button);
  }

  gtk_box_pack_start (GTK_BOX (vbox), file_name, 1, 1, 0);
  gtk_box_pack_start (GTK_BOX (vbox), box, 1, 1, 0);
  gtk_box_pack_start (GTK_BOX (vbox), output, 1, 1, 0);

  gtk_signal_connect (GTK_OBJECT (find_file_button), "clicked",
		      GTK_SIGNAL_FUNC (find_file), NULL);

  gtk_signal_connect (GTK_OBJECT (play_button), "clicked",
		      GTK_SIGNAL_FUNC (play), NULL);
  
  gtk_signal_connect (GTK_OBJECT (stop_button), "clicked",
		      GTK_SIGNAL_FUNC (stop), NULL);
          
  gtk_container_add (GTK_CONTAINER (window), vbox);
            
  /* the final step is to display this newly created widget... */
  gtk_widget_show (find_file_button);
  gtk_widget_show (stop_button);
  gtk_widget_show (play_button);
  gtk_widget_show (file_name);
  gtk_widget_show (output);
  gtk_widget_show (optargs);
  gtk_widget_show (box);
  gtk_widget_show (vbox);
  
  gtk_widget_show (window);
          
  gtk_main ();
          
  return 0;
}

/* EOF */
