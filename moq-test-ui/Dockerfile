FROM node:22-alpine

RUN apk add --no-cache bash

WORKDIR /app

COPY package.json package-lock.json* ./
RUN npm install --production

COPY src/ ./src/
COPY public/ ./public/
COPY tools/ ./tools/

RUN mkdir -p /app/results

EXPOSE 3000

CMD ["node", "src/server.js"]
