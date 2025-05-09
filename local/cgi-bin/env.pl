#!/usr/bin/perl
print "Content-Type: text/html\n\n";
print "<html>";
print "<head><title>Print ENV</title></head>";
print "<link rel="icon" href="/images/favicon.ico" type="image/x-icon">"
print "<body>";
print "<h1>Environment Variables</h1>";
print "<ul>";
foreach my $key (sort keys %ENV) {
    print "<li><strong>$key:</strong> $ENV{$key}</li>";
}
print "</ul>";
print "</body></html>";