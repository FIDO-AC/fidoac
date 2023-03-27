FROM node:alpine

COPY . /tmp/build
COPY ./keys /app

RUN cd /tmp/build && npm install
RUN cd /tmp/build && npm run build
RUN cp -R /tmp/build/dist/* /app
RUN ls /app
WORKDIR /app

CMD ["node", "index.js"]