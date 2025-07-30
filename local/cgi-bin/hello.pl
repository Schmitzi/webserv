#!/usr/bin/perl
print "Content-Type: text/html\n\n";
print "<html>\n";
print "<head>\n";
print "<title>Perl CGI</title>\n";
print "<link rel=\"icon\" href=\"/images/favicon.ico\" type=\"image/x-icon\">\n";
print "</head>\n";
print " <body>\n";
print "     <h1>Hello from Perl CGI!</h1>\n";
print "         <div><a href=\"../cgi.html\">Back</a></div>\n";

# Print request method
print "         <p><strong>Request Method:</strong> $ENV{'REQUEST_METHOD'}</p>\n";

# Print query string
print "         <p><strong>Query String:</strong> $ENV{'QUERY_STRING'}</p>\n";

# Print content type and length for POST requests
if ($ENV{'REQUEST_METHOD'} eq 'POST') {
    print "  <p><strong>Content Type:</strong> $ENV{'CONTENT_TYPE'}</p>\n";
    print "  <p><strong>Content Length:</strong> $ENV{'CONTENT_LENGTH'}</p>\n";
    
    # Basic POST data handling (simpler than using CGI module)
    if ($ENV{'CONTENT_LENGTH'} > 0) {
        my $post_data;
        read(STDIN, $post_data, $ENV{'CONTENT_LENGTH'});
        print " <h2>POST Data (raw):</h2>\n";
        print "     <pre>$post_data</pre>\n";
    }
}

print " </body>\n</html>\n";