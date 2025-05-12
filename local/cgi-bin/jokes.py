#!/usr/bin/env python3
import random
import datetime as dt

# Initialize programming jokes database
jokes = [
    {"text": "Why do programmers prefer dark mode? Because light attracts bugs.", "category": "General"},
    {"text": "A SQL query walks into a bar, walks up to two tables, and asks, 'Can I join you?'", "category": "SQL"},
    {"text": "Why do Java developers wear glasses? Because they don't C#.", "category": "Language Wars"},
    {"text": "How many programmers does it take to change a light bulb? None, that's a hardware problem.", "category": "Classic"},
    {"text": "What's the object-oriented way to become wealthy? Inheritance.", "category": "OOP"},
    {"text": "Why was the JavaScript developer sad? Because he didn't know how to 'null' his feelings.", "category": "JavaScript"},
    {"text": "Two bytes meet. The first byte asks, 'Are you ill?' The second byte replies, 'No, just feeling a bit off.'", "category": "Bit Humor"},
    {"text": "Why did the functions stop calling each other? They had too many arguments.", "category": "Functions"},
    {"text": "Why don't bachelors like Git? Because they're afraid to commit.", "category": "Git"},
    {"text": "I would tell you a UDP joke, but you might not get it.", "category": "Networking"},
    {"text": "Debugging is like being the detective in a crime movie where you're also the murderer.", "category": "Debugging"},
    {"text": "The best thing about a Boolean is that even if you're wrong, you're only off by a bit.", "category": "Data Types"},
    {"text": "Why do programmers always mix up Halloween and Christmas? Because Oct 31 == Dec 25.", "category": "Number Systems"},
    {"text": "An SEO expert walks into a bar, bars, pub, tavern, public house, Irish pub, drinks, beer, alcohol...", "category": "Web"},
    {"text": "Why do programmers hate nature? It has too many bugs.", "category": "Bugs"}
]

# Select random joke
selected = random.choice(jokes)

# Get current time
current_time = dt.datetime.now().strftime("%A, %B %d, %Y at %H:%M:%S")

# Output HTTP headers
print("Content-Type: text/html\r\n\r\n")

# Output HTML page with the joke
html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Programming Joke of the Day</title>
    <link rel="icon" href="/images/favicon.ico" type="image/x-icon">
    <style>
        body {{
            font-family: 'Courier New', monospace;
            max-width: 800px;
            margin: 0 auto;
            padding: 2rem;
            text-align: center;
            background-color: #1e1e1e;
            color: #d4d4d4;
        }}
        h1 {{
            color: #569cd6;
        }}
        .joke-container {{
            background-color: #2d2d2d;
            border-radius: 10px;
            padding: 2rem;
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
            margin-top: 2rem;
            border-left: 4px solid #608b4e;
        }}
        .joke {{
            font-size: 1.5rem;
            line-height: 1.4;
            margin-bottom: 1rem;
            color: #dcdcaa;
        }}
        .category {{
            font-style: italic;
            font-size: 1rem;
            color: #ce9178;
            background-color: #3a3a3a;
            display: inline-block;
            padding: 3px 10px;
            border-radius: 15px;
            margin-top: 10px;
        }}
        .refresh-btn {{
            background-color: #608b4e;
            color: white;
            border: none;
            padding: 10px 20px;
            margin-top: 2rem;
            border-radius: 5px;
            cursor: pointer;
            font-size: 1rem;
            transition: background-color 0.3s;
            font-family: 'Courier New', monospace;
        }}
        .refresh-btn:hover {{
            background-color: #6a9955;
        }}
        .timestamp {{
            margin-top: 2rem;
            font-size: 0.8rem;
            color: #777;
        }}
        .code-comment {{
            color: #608b4e;
            margin-bottom: 0;
            text-align: left;
            margin-left: 2rem;
        }}
        .blink {{
            animation: blink-animation 1s steps(5, start) infinite;
        }}
        @keyframes blink-animation {{
            to {{
                visibility: hidden;
            }}
        }}
    </style>
</head>
<body>
    <p class="code-comment">// Your daily dose of programmer humor</p>
    <h1>$ Programming Joke.exe <span class="blink">_</span></h1>
    <div class="pt-4 border-t border-gray-200">
    <a href="../cgi.html" class="block text-center text-blue-500 hover:underline">
        Back to CGI
    </a>
    </div>
    <div class="joke-container">
        <p class="joke">"{selected['text']}"</p>
        <p class="category">#{selected['category']}</p>
    </div>
    
    <button class="refresh-btn" onclick="location.reload();">Run joke.next()</button>
    <p class="timestamp">Generated on {current_time} | Runtime: {random.randint(1, 100)}ms</p>
</body>
</html>
"""

print(html)