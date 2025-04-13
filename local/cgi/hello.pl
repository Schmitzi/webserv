#!/usr/bin/perl
use strict;
use warnings;

print "Content-Type: text/html\n\n";
print <<HTML;
<!DOCTYPE html>
<html>
<head>
    <title>Perl CGI</title>
</head>
<body>
    <div>
        <a href="../cgi.html">Back</a>
    </div>
    <h1>Hello from Perl CGI!</h1>
</body>
</html>
HTML