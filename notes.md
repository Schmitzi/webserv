[nm -u ./webserv]

-> change atoi to std::...

-> can we use epoll_create1? (jakob said we should use 2, subject only specifies epoll_create)

<!-- -> if all bind() fail: dont start webserv -->

<!-- -> check error_pages in config (should be >= 400) -->

-> GET /dev/urandom returns 404, should not be 404

-> GET /for location / without GET in limit_except still returns index.html

-> combine upload_store with location name and root

-> combine error_pages with root

-> add check for index in config to only take 1 file or change it to be able to use more than one (just dont ignore)

-> open fails because of 403 but sends 500 because it wasnt checked properly

-> if no default_server specified take first one as default

<!-- -> check filesize (bytes to be received) before receiving bytes! -->