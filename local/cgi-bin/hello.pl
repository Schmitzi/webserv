#!/usr/bin/perl
print "Content-Type: text/html\n\n";
print "<html>";
print "<head>";
print "<title>Perl CGI</title>";
print "<link rel=\"icon\" href=\"/images/favicon.ico\" type=\"image/x-icon\">";
print "</head>";
print "<body>";
print "<h1>Hello from Perl CGI!</h1>";
print "<div><a href=\"../cgi.html\">Back</a></div>";

# Print request method
print "<p><strong>Request Method:</strong> $ENV{'REQUEST_METHOD'}</p>";

# Print query string
print "<p><strong>Query String:</strong> $ENV{'QUERY_STRING'}</p>";

# Print content type and length for POST requests
if ($ENV{'REQUEST_METHOD'} eq 'POST') {
    print "<p><strong>Content Type:</strong> $ENV{'CONTENT_TYPE'}</p>";
    print "<p><strong>Content Length:</strong> $ENV{'CONTENT_LENGTH'}</p>";
    
    # Basic POST data handling (simpler than using CGI module)
    if ($ENV{'CONTENT_LENGTH'} > 0) {
        my $post_data;
        read(STDIN, $post_data, $ENV{'CONTENT_LENGTH'});
        print "<h2>POST Data (raw):</h2>";
        print "<pre>$post_data</pre>";
    }
}

print "</body></html>";