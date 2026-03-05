// Chatbot FAQ Database
const faqDatabase = {
    'what is webserv': {
        answer: "Webserv is a HTTP/1.1 web server written from scratch in C++ 98. It's a 42 School project that demonstrates understanding of network programming, the HTTP protocol, and non-blocking I/O operations.",
        suggestions: ['How does it work?', 'What technologies are used?', 'What features does it have?']
    },
    'how does it work': {
        answer: "Webserv uses an event-driven architecture with non-blocking I/O. It creates sockets, listens for connections, and uses poll/select/epoll/kqueue to multiplex I/O operations. When a request arrives, it parses the HTTP headers, routes to the appropriate handler, processes the request, and sends back an HTTP response.",
        suggestions: ['What is poll/epoll?', 'Tell me about the architecture', 'What about performance?']
    },
    'what methods are supported': {
        answer: "Webserv supports the essential HTTP methods: GET (retrieve resources), POST (create/submit data), and DELETE (remove resources). These cover the fundamental operations needed for a web server.",
        suggestions: ['How do I test methods?', 'What about PUT or PATCH?', 'Tell me about CGI']
    },
    'what is cgi': {
        answer: "CGI (Common Gateway Interface) allows the server to execute external scripts (PHP, Python, Perl) to generate dynamic content. When a CGI request comes in, webserv uses fork() and execve() to run the script, passes environment variables, and returns the output to the client.",
        suggestions: ['How to configure CGI?', 'What languages are supported?', 'Tell me about file uploads']
    },
    'file upload': {
        answer: "Webserv handles file uploads via POST requests with multipart/form-data encoding. It parses the request body, extracts file data in chunks (non-blocking), validates file size limits, sanitizes filenames to prevent security issues, and saves files to the configured upload directory.",
        suggestions: ['How to test uploads?', 'What about file size limits?', 'Tell me about security']
    },
    'configuration': {
        answer: "Webserv uses NGINX-style configuration files. You define server blocks with directives like 'listen' (port), 'server_name', 'root' (document directory), 'location' blocks for routes, 'allow_methods', and 'error_page' for custom error pages. Multiple servers can run on different ports.",
        suggestions: ['Show me an example', 'How to add virtual hosts?', 'What about error pages?']
    },
    'non-blocking': {
        answer: "Non-blocking I/O means operations don't wait for completion. Instead of blocking on read/write, webserv uses poll/epoll/kqueue to monitor multiple file descriptors simultaneously. When data is ready, the event loop processes it. This allows handling thousands of concurrent connections efficiently.",
        suggestions: ['What is poll/epoll?', 'Why is this important?', 'Tell me about performance']
    },
    'poll epoll kqueue': {
        answer: "poll(), epoll(), and kqueue() are system calls for I/O multiplexing. They let you monitor multiple file descriptors and get notified when they're ready for I/O. poll() is standard but less efficient; epoll (Linux) and kqueue (BSD/macOS) are more scalable for thousands of connections.",
        suggestions: ['Which one does webserv use?', 'How does it work?', 'What about performance?']
    },
    'performance': {
        answer: "Webserv is designed for high performance: non-blocking I/O handles thousands of concurrent connections, event-driven architecture minimizes CPU usage, efficient parsing reduces overhead, and proper resource management prevents memory leaks. It can handle 10,000+ simultaneous connections with sub-millisecond response times.",
        suggestions: ['How to test performance?', 'What about benchmarks?', 'Compare to NGINX?']
    },
    'c++ 98': {
        answer: "C++ 98 is the ISO C++ standard from 1998. It lacks modern features like smart pointers, auto keyword, lambdas, and move semantics. This project uses C++ 98 to demonstrate understanding of manual memory management, proper resource handling (RAII), and traditional OOP design patterns.",
        suggestions: ['Why C++ 98?', 'What challenges does this create?', 'Tell me about the architecture']
    },
    'architecture': {
        answer: "Webserv uses a modular architecture: ConfigParser reads config files, Server manages sockets and event loop, RequestHandler parses HTTP requests, ResponseBuilder constructs responses, and CGIHandler executes scripts. Each component has a single responsibility, making the code maintainable and testable.",
        suggestions: ['Tell me about components', 'How does the event loop work?', 'What about error handling?']
    },
    'error handling': {
        answer: "Webserv handles errors gracefully: invalid requests get proper HTTP error codes (400, 404, 500, etc.), custom error pages can be configured, connection timeouts are detected, and the server never crashes from client errors. All errors are logged for debugging.",
        suggestions: ['What error codes are supported?', 'How to customize error pages?', 'Tell me about timeouts']
    },
    'security': {
        answer: "Security features include: path traversal prevention (can't access files outside root), filename sanitization for uploads, request size limits to prevent DoS, timeout handling to free resources, and proper input validation. Always validate and sanitize user input!",
        suggestions: ['Tell me about file uploads', 'What about DoS protection?', 'How are paths validated?']
    },
    'virtual hosts': {
        answer: "Virtual hosts allow multiple websites on one server. Each server block in the config can have a different 'server_name' and 'listen' port. Webserv matches the incoming Host header to the correct virtual host and serves content from that server's root directory.",
        suggestions: ['Show me an example', 'How to configure multiple ports?', 'Tell me about configuration']
    },
    'session management': {
        answer: "Webserv can track user sessions using cookies. It generates unique session IDs, sends them as Set-Cookie headers, parses Cookie headers from requests, and maintains session state. Sessions can have timeouts for security.",
        suggestions: ['How do cookies work?', 'What about session timeout?', 'Tell me about security']
    },
    'testing': {
        answer: "Test webserv with: 1) curl commands for quick HTTP requests, 2) this web interface for interactive testing, 3) browser testing for real-world usage, 4) telnet for low-level protocol testing, 5) siege/ab for load testing. The project includes comprehensive test suites.",
        suggestions: ['Show me curl examples', 'How to load test?', 'What about the web interface?']
    }
};

// Toggle chatbot window
function toggleChatbot() {
    const chatbot = document.getElementById('chatbot');
    const tooltip = document.querySelector('.chatbot-tooltip');
    chatbot.classList.toggle('open');
    
    // Hide tooltip when window opens
    if (chatbot.classList.contains('open') && tooltip) {
        tooltip.style.opacity = '0';
    }
}

// Send user message
function sendMessage() {
    const input = document.getElementById('chatInput');
    const message = input.value.trim();
    
    if (!message) return;

    // Add user message
    addMessage(message, 'user');
    input.value = '';

    // Process and respond
    setTimeout(() => {
        respondToQuestion(message);
    }, 500);
}

// Quick question from suggestion button
function askQuestion(question) {
    addMessage(question, 'user');
    setTimeout(() => {
        respondToQuestion(question);
    }, 500);
}

// Add message to chat
function addMessage(text, sender) {
    const messagesDiv = document.getElementById('chatMessages');
    const messageDiv = document.createElement('div');
    messageDiv.className = 'chatbot-message';
    
    if (sender === 'user') {
        messageDiv.innerHTML = `
            <div class="chatbot-bubble user">${text}</div>
            <div class="chatbot-avatar user">👤</div>
        `;
        messageDiv.style.flexDirection = 'row-reverse';
    } else {
        messageDiv.innerHTML = `
            <div class="chatbot-avatar bot">🤖</div>
            <div class="chatbot-bubble">${text}</div>
        `;
    }
    
    messagesDiv.appendChild(messageDiv);
    messagesDiv.scrollTop = messagesDiv.scrollHeight;
}

// Process question and generate response
function respondToQuestion(question) {
    const normalizedQ = question.toLowerCase().trim();
    
    // Find matching FAQ
    let response = null;
    for (const [key, value] of Object.entries(faqDatabase)) {
        if (normalizedQ.includes(key) || key.includes(normalizedQ.split(' ').slice(0, 3).join(' '))) {
            response = value;
            break;
        }
    }

    if (response) {
        const messagesDiv = document.getElementById('chatMessages');
        const messageDiv = document.createElement('div');
        messageDiv.className = 'chatbot-message';
        
        let suggestionsHTML = '';
        if (response.suggestions && response.suggestions.length > 0) {
            suggestionsHTML = '<div class="chatbot-suggestions">';
            response.suggestions.forEach(sug => {
                suggestionsHTML += `<button class="chatbot-suggestion" onclick="askQuestion('${sug}')">${sug}</button>`;
            });
            suggestionsHTML += '</div>';
        }
        
        messageDiv.innerHTML = `
            <div class="chatbot-avatar bot">🤖</div>
            <div>
                <div class="chatbot-bubble">${response.answer}</div>
                ${suggestionsHTML}
            </div>
        `;
        
        messagesDiv.appendChild(messageDiv);
        messagesDiv.scrollTop = messagesDiv.scrollHeight;
    } else {
        // Default response when no match found
        addMessage("I'm not sure about that. Try asking about: what webserv is, how it works, supported methods, CGI, file uploads, configuration, performance, or architecture.", 'bot');
    }
}
