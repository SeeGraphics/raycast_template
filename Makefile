CC ?= cc
SRC := src/main.c
OBJ := $(SRC:src/%.c=build/%.o)

TEXTURED_SRC := src/textured.c
TEXTURED_OBJ := $(TEXTURED_SRC:src/%.c=build/%.o)

SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS := $(shell sdl2-config --libs 2>/dev/null)
SDL_IMAGE_CFLAGS := $(shell pkg-config SDL2_image --cflags 2>/dev/null)
SDL_IMAGE_LIBS := $(shell pkg-config SDL2_image --libs 2>/dev/null)

CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -O2
CFLAGS += $(SDL_CFLAGS) $(SDL_IMAGE_CFLAGS)
LDLIBS := $(if $(SDL_LIBS),$(SDL_LIBS),-lSDL2) -lm
TEXTURED_LDLIBS := $(LDLIBS) \
	$(if $(SDL_IMAGE_LIBS),$(SDL_IMAGE_LIBS),-lSDL2_image)

TARGET := build/raycast
TEXTURED_TARGET := build/raycast_textured

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

.PHONY: textured
textured: $(TEXTURED_TARGET)
	./$(TEXTURED_TARGET)

$(TEXTURED_TARGET): $(TEXTURED_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(TEXTURED_OBJ) -o $@ $(TEXTURED_LDLIBS)

.PHONY: run
run: $(TARGET)
	./$(TARGET)

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf build $(TARGET)
