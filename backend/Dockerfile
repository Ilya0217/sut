# Этап сборки
FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git pkg-config ca-certificates \
    libssl-dev libpq-dev libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt .
COPY src/ src/

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . -j$(nproc)

# Этап выполнения
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3t64 libpq5 libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /app/build/sut_backend /usr/local/bin/sut_backend

EXPOSE 5000

CMD ["sut_backend"]
