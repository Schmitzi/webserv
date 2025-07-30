#!/usr/bin/bash

echo "Content-Type: text/html"
echo ""
echo "<html>"
echo "<head>"
echo "<title>Bash CGI</title>"
echo "<style>body { font-family: Arial, sans-serif; margin: 20px; }</style>"
echo "<link rel="icon" href="/images/favicon.ico" type="image/x-icon">"
echo "</head>"
echo "<body>"
echo "<h1>Hello from Bash CGI!</h1>"
echo "<div><a href=\"../cgi.html\">Back</a></div>"

# Print request method
echo "<p><strong>Request Method:</strong> $REQUEST_METHOD</p>"

# Print query string
echo "<p><strong>Query String:</strong> $QUERY_STRING</p>"

# Handle query parameters (simple parsing)
if [ -n "$QUERY_STRING" ]; then
    echo "<h2>Query Parameters:</h2>"
    echo "<ul>"
    # Split the query string at ampersands
    IFS='&' read -ra PARAMS <<< "$QUERY_STRING"
    for param in "${PARAMS[@]}"; do
        # Split each parameter at equals sign
        IFS='=' read -r key value <<< "$param"
        echo "<li><strong>$key:</strong> $value</li>"
    done
    echo "</ul>"
fi

# Handle POST data
if [ "$REQUEST_METHOD" = "POST" ]; then
    echo "<p><strong>Content Type:</strong> $CONTENT_TYPE</p>"
    echo "<p><strong>Content Length:</strong> $CONTENT_LENGTH</p>"
    
    if [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ]; then
        echo "<h2>POST Data (raw):</h2>"
        echo "<pre>"
        # Read POST data from stdin
        dd bs=$CONTENT_LENGTH count=1 2>/dev/null
        echo "</pre>"
    fi
fi

echo "</body>"
echo "</html>"
exit 0