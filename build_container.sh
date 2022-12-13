docker buildx build --platform="linux/amd64,linux/arm64" -t docker.io/bniemczyk/bnirc build .
docker push docker.io/bniemczyk/bnirc
