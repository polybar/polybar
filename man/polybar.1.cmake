.TH polybar 1 @LAST_COMMIT_DATE@ "polybar @APP_VERSION@" "User Manual"
.SH NAME
polybar \- A fast and easy-to-use tool status bar
.SH SYNOPSIS
\fBpolybar\fR [\fIOPTION\fR]... \fIBAR\fR
.SH DESCRIPTION
Polybar aims to help users build beautiful and highly customizable status bars for their desktop environment, without the need of having a black belt in shell scripting.
.TP
\fB\-h\fR, \fB\-\-help\fR
Display help text and exit
.TP
\fB\-v\fR, \fB\-\-version\fR
Display build details and exit
.TP
\fB\-l\fR, \fB\-\-log\fR=\fILEVEL\fR
Set the logging verbosity (default: \fBWARNING\fR)
.br
\fILEVEL\fR is one of: error, warning, info, trace
.TP
\fB\-q\fR, \fB\-\-quiet\fR
Be quiet (will override -l)
.TP
\fB\-c\fR, \fB\-\-config\fR=\fIFILE\fR
Specify the path to the configuration file. By default, the configuration file is loaded from:
.RS 10
.P
\fB$XDG_CONFIG_HOME/polybar/config\fR
.br
\fB$HOME/.config/polybar/config\fR
.P
.RE
.TP
\fB\-r\fR, \fB\-\-reload\fR
Reload the application when the config file has been modified
.TP
\fB\-d\fR, \fB\-\-dump\fR=\fIPARAM\fR
Print the value of the specified parameter \fIPARAM\fR in bar section and exit
.TP
\fB\-m\fR, \fB\-\-list\-monitors\fR
Print list of available monitors and exit
.TP
\fB\-w\fR, \fB\-\-print\-wmname\fR
Print the generated \fIWM_NAME\fR and exit
.TP
\fB\-s\fR, \fB\-\-stdout\fR
Output the data to stdout instead of drawing it to the X window
.TP
\fB\-p\fR, \fB\-\-png\fR=\fIFILE\fR
Save png snapshot to \fIFILE\fR after running for 3 seconds
.sp
.SH AUTHOR
Michael Carlberg <c@rlberg.se>
.br
Contributors can be listed on GitHub.
.SH REPORTING BUGS
Report issues on GitHub <https://github.com/jaagr/polybar>
.SH SEE ALSO
Full documentation at: <https://github.com/jaagr/polybar>
.br
Project wiki: <https://github.com/jaagr/polybar/wiki>
