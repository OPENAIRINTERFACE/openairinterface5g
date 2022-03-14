FROM nginx:alpine

COPY dist/lsd-ngx /usr/share/nginx/html/

COPY nginx.conf /etc/nginx/conf.d/default.conf

# sed replace $PORT by $PORT at runtime (on heroku with the correct port)
CMD sed -i -e 's/$PORT/'"$PORT"'/g' /etc/nginx/conf.d/default.conf && nginx -g 'daemon off;'
