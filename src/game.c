#define _CRT_SECURE_NO_WARNINGS

#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "graphics.h"
#include "utils.h"

// Inicializa a janela do jogo e define o FPS.
void InitGame(GameState *state, GameTextures *textures, GameSounds *sounds) {
    SetTargetFPS(TARGET_FPS);
    InitWindow(BASE_WIDTH_INT, BASE_HEIGHT_INT, GAME_TITLE);
    InitializeTextures(textures);
    InitializeSounds(sounds);
    InitializeGameState(state);
    SetExitKey(0);  // Desabilita a saída com ESC
}

// Inicializa o estado do jogo com valores padrão.
void InitializeGameState(GameState *state) {
    // Inicializa valores básicos do estado do jogo.
    state->titleScreen = true;
    state->gameOver = false;
    state->pause = false;
    state->musicPaused = false;
    state->money = INITIAL_MONEY;
    state->mousePick = 1;  // Valor padrão quando nada está selecionado.

    // Inicializa custos dos personagens.
    state->characterCost[CHIMPANZINI_FRAME_ID] = CHIMPANZINI_COST;
    state->characterCost[TRALALERO_FRAME_ID] = TRALALERO_COST;
    state->characterCost[SAHUR_FRAME_ID] = SAHUR_COST;
    state->characterCost[LIRILI_FRAME_ID] = LIRILI_COST;
    state->characterCost[BOMBARDINI_FRAME_ID] = BOMBARDINI_COST;

    // Inicializa o valor de cooldown dos personagens.
    state->characterCD[CHIMPANZINI_FRAME_ID] = CHIMPANZINI_CD;
    state->characterCD[TRALALERO_FRAME_ID] = TRALALERO_CD;
    state->characterCD[SAHUR_FRAME_ID] = SAHUR_CD;
    state->characterCD[LIRILI_FRAME_ID] = LIRILI_CD;
    state->characterCD[BOMBARDINI_FRAME_ID] = BOMBARDINI_CD;

    // Inicializa o bool e o contador de frames de verificação de personagem em cooldown
    for (int i = 0; i < 5; i++) {
        state->inCooldown[i] = false;
        state->frameCounterCD[i] = 0;
    }
    // Inicializa valores das estatisticas
    state->currentWave = 1;
    state->charactersBought = 0;
    state->charactersSold = 0;
    state->charactersLost = 0;
    state->moneyBagsCollected = 0;
    state->moneyBagsMissed = 0;
    state->enemiesKilled = 0;
    state->currentPoints = 0;

    // Inicializa contadores de animação e estado da bolsa de dinheiro.
    state->frameCounterPisc = 0;
    state->frameCounterIdle = 0;
    state->pisc = 0;                     // Usado para o efeito de piscar da bolsa.
    state->moneyBag = false;             // Indica se a bolsa de dinheiro está ativa.
    state->randomizePointBagPos = true;  // Controla se a posição da bolsa deve ser randomizada.
    state->piscBool = true;              // Alterna para o efeito de piscar.
    state->randomNumX = 0;               // Posição X da bolsa de dinheiro.
    state->randomNumY = 0;               // Posição Y da bolsa de dinheiro.

    // Seta todas as Tiles como 1 (gramado padrão), exceto as da coluna 0, que possuem valor 0 (botões).
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLUMNS; c++) {
            state->tiles[r][c] = 1;
            state->tiles[r][0] = 0;  // Coluna 0 é para botões/seleção
        }
    }

    // Insere os códigos dos personagens no seletor.
    // Os IDs dos personagens são definidos em config.h (ex: CHIMPANZINI_ID = 16).
    for (int f = CHIMPANZINI_ID; f <= BOMBARDINI_ID; f++) {
        state->frame[f - CHIMPANZINI_ID] = f;
    }

    // Inicializa arrays de personagens com valores default.
    memset(state->chimpanzini, 0, sizeof(state->chimpanzini));
    memset(state->tralalero, 0, sizeof(state->tralalero));
    memset(state->sahur, 0, sizeof(state->sahur));
    memset(state->lirili, 0, sizeof(state->lirili));
    memset(state->bombardini, 0, sizeof(state->bombardini));
}

// Atualiza a lógica principal do jogo por frame.
void UpdateGame(GameState *state) {
    // Incrementa o contador de frames para animações e inatividade.
    state->frameCounterIdle++;

    // Reseta FrameCounterIdle após 1 minuto (60 segundos * 60 FPS = 3600 frames).
    if (state->frameCounterIdle >= TimeToFrames(60)) {
        state->frameCounterIdle = 0;
    }

    UpdateCharacters(state);                   // Atualiza o estado e animações dos personagens
    UpdateProjectiles(state, GetFrameTime());  // Atualiza a posição dos projéteis
    UpdateMoneyBag(state);                     // Atualiza a lógica da bolsa de dinheiro
}

// Atualiza os estados e animações dos personagens.
void UpdateCharacters(GameState *state) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLUMNS; c++) {
            // Lógica para remover personagens com HP <= 0
            if (state->chimpanzini[r][c].exists && state->chimpanzini[r][c].hp <= 0) {
                state->chimpanzini[r][c].exists = false;
                state->tiles[r][c] = 1;      // Retorna a tile para o estado padrão.
                state->charactersLost += 1;  // Incremente o contador de personagens perdidos
            }
            if (state->tralalero[r][c].exists && state->tralalero[r][c].hp <= 0) {
                state->tralalero[r][c].exists = false;
                state->tiles[r][c] = 1;
            }
            if (state->sahur[r][c].exists && state->sahur[r][c].hp <= 0) {
                state->sahur[r][c].exists = false;
                state->tiles[r][c] = 1;
            }
            if (state->lirili[r][c].exists && state->lirili[r][c].hp <= 0) {
                state->lirili[r][c].exists = false;
                state->tiles[r][c] = 1;
            }
            if (state->bombardini[r][c].exists && state->bombardini[r][c].hp <= 0) {
                state->bombardini[r][c].exists = false;
                state->tiles[r][c] = 1;
            }

            // Atualiza o frame de animação dos personagens a cada 8 frames de inatividade.
            if (state->frameCounterIdle % 8 == 0) {
                if (state->chimpanzini[r][c].exists) state->chimpanzini[r][c].idle++;
                if (state->tralalero[r][c].exists) state->tralalero[r][c].idle++;
                if (state->sahur[r][c].exists) state->sahur[r][c].idle++;
                // Bombardini só anima se não estiver "pronto" (carregando a bomba)
                if (state->bombardini[r][c].exists && !state->bombardini[r][c].ready) state->bombardini[r][c].idle++;
            }

            // Lógica de comportamento do Chimpanzini (geração de dinheiro)
            if (state->chimpanzini[r][c].exists) {
                if (!state->chimpanzini[r][c].shining) {  // Animação de idle
                    if (state->chimpanzini[r][c].idle == 3) {
                        state->chimpanzini[r][c].idle = 0;
                        state->chimpanzini[r][c].loop++;
                    }
                    if (state->chimpanzini[r][c].loop == 60) {  // Após X loops, começa a brilhar
                        state->chimpanzini[r][c].shining = true;
                        state->chimpanzini[r][c].idle = 4;  // Inicia animação de brilho
                    }
                } else {  // Animação de brilho
                    if (state->chimpanzini[r][c].idle == 7) {
                        state->chimpanzini[r][c].idle = 4;  // Mantém no loop de brilho
                        state->chimpanzini[r][c].loop = 0;  // Reseta loop para próxima geração
                    }
                }
            }

            // Lógica de comportamento do Tralalero (ataque de projétil)
            if (state->tralalero[r][c].exists) {
                if (!state->tralalero[r][c].attacking) {  // Animação de idle
                    if (state->tralalero[r][c].idle == 3) {
                        state->tralalero[r][c].idle = 0;
                        state->tralalero[r][c].loop++;
                    }
                    if (state->tralalero[r][c].loop == 20) {  // Após X loops, começa a atacar
                        state->tralalero[r][c].attacking = true;
                        state->tralalero[r][c].idle = 4;  // Inicia animação de ataque
                        state->tralalero[r][c].loop = 0;  // Reseta loop
                    }
                } else {  // Animação de ataque
                    if (state->tralalero[r][c].idle == 7) {
                        state->tralalero[r][c].attacking = false;
                        state->shouldPlaySound = true;
                        state->soundToPlay = SOUND_PROJECTILE;
                        state->tralalero[r][c].projecB = true;  // Ativa o projétil
                        state->tralalero[r][c].idle = 0;        // Volta para idle
                        // Define a posição inicial do projétil
                        state->tralalero[r][c].projecX = (GRID_MARGIN_X + 20) + (c * 96) + 35;
                        state->tralalero[r][c].projecY = GRID_MARGIN_Y + (r * 78);
                    }
                }
            }

            // Lógica de comportamento do Sahur (idle simples)
            if (state->sahur[r][c].exists) {
                if (state->sahur[r][c].idle == 3) {
                    state->sahur[r][c].idle = 0;
                }
            }

            // Lógica de comportamento do Lirili (mudança de sprite conforme HP)
            if (state->lirili[r][c].exists) {
                if (state->lirili[r][c].hp < 100 && state->lirili[r][c].hp >= 75) {
                    state->lirili[r][c].idle = 1;
                } else if (state->lirili[r][c].hp < 75 && state->lirili[r][c].hp >= 50) {
                    state->lirili[r][c].idle = 2;
                } else if (state->lirili[r][c].hp < 50) {
                    state->lirili[r][c].idle = 3;
                } else {
                    state->lirili[r][c].idle = 0;  // Estado inicial
                }
            }

            // Lógica de comportamento do Bombardini (carregamento da bomba)
            if (state->bombardini[r][c].exists && !state->bombardini[r][c].ready) {
                if (state->bombardini[r][c].idle == 3) {
                    state->bombardini[r][c].idle = 0;
                    state->bombardini[r][c].loop++;
                }
                if (state->bombardini[r][c].loop == 300) {  // Após X loops, fica "pronto"
                    state->bombardini[r][c].ready = true;
                    state->bombardini[r][c].idle = 4;  // Inicia animação de pronto
                }
            }
        }
    }
}

// Atualiza a lógica dos projéteis.
void UpdateProjectiles(GameState *state, float deltaTime) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLUMNS; c++) {
            if (state->tralalero[r][c].projecB) {
                // Move o projétil
                state->tralalero[r][c].projecX += PROJECTILE_SPEED * deltaTime;

                // Se o projétil sair da tela, reseta-o
                if (state->tralalero[r][c].projecX > BASE_WIDTH_INT) {
                    state->tralalero[r][c].projecX = (GRID_MARGIN_X + 20) + (c * 96) + 35;  // Posição inicial
                    state->tralalero[r][c].projecB = false;                                 // Desativa o projétil
                }
            }
            // Lógica para bombas do Bombardini (se houver)
        }
    }
}

// Atualiza a lógica da bolsa de dinheiro aleatória.
void UpdateMoneyBag(GameState *state) {
    // Lógica da bolsa de dinheiro aleatória.
    // Alterna piscBool para fazer a bolsa piscar.
    if (state->frameCounterPisc % 18 == 0) {
        state->piscBool = !state->piscBool;
    }

    // Define o tamanho da bolsa para o efeito de piscar.
    if (state->piscBool == true) {
        state->pisc = 3;
    } else {
        state->pisc = 0;
    }

    // Gera bolsa aleatória se ainda não houver uma ativa.
    // srand(time(NULL)) deve ser chamado apenas uma vez no início do programa (em main.c ou InitGame).
    // Aqui, vamos usar um número aleatório simples para decidir se a bolsa aparece.
    if (state->randomizePointBagPos) {
        // Gera um número aleatório para decidir se a bolsa aparece
        int randomChance = rand() % MONEY_BAG_RANDOMNESS;  // Ajuste este valor para mudar a frequência
        // Debugging.
        char RandomText[10];
        sprintf(RandomText, "%d", randomChance);
        DrawText(RandomText, 100, 80, FONT_SIZE, BLACK);
        if (randomChance == 0 && !state->moneyBag) {
            state->moneyBag = true;
            // Define a posição aleatória da bolsa
            state->randomNumX = rand() % (BASE_WIDTH_INT - 150) + 50;    // Evita bordas.
            state->randomNumY = rand() % (BASE_HEIGHT_INT - 200) + 150;  // Evita HUD superior
            state->frameCounterPisc = TimeToFrames(30);                  // Duração da bolsa (30 segundos)
            state->randomizePointBagPos = false;                         // Impede que a posição seja randomizada novamente enquanto ativa
        }
    }

    if (state->moneyBag) {
        state->frameCounterPisc--;  // Decrementa o contador de tempo da bolsa

        // Se o tempo da bolsa acabar, ela desaparece.
        if (state->frameCounterPisc <= 0) {
            state->randomizePointBagPos = true;  // Permite nova randomização
            state->moneyBag = false;
            state->moneyBagsMissed += 1;  // Incrementa o contador de bolsas perdidas
        }
    }
}

// Processa a entrada do usuário que afeta o estado do jogo (cliques em botões, tiles).
void ProcessGameInput(GameState *state, Vector2 mousePos, int screenWidth, int screenHeight) {
    // Lógica da tela de título
    if (state->titleScreen) {
        Rectangle playDest = ScaleRectTo720p((int)1280 / 2.5 - 5, (int)720 / 2, 210, BASE_FONT_SIZE, screenWidth, screenHeight);
        if (CheckCollisionPointRec(mousePos, playDest)) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                state->titleScreen = false;  // Sai da tela de título
            }
        }
        return;  // Não processa mais nada se estiver na tela de título
    }
    // Pausa e despausa o jogo ao apertar ESC ou P
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        state->pause = !state->pause;
        state->mousePick = 1;
    }
    // Lógica do botão "SELL"
    Rectangle sellDest = ScaleRectTo720p(SELL_POS_X - 5, SELL_POS_Y, 110, 50, screenWidth, screenHeight);
    if (((CheckCollisionPointRec(mousePos, sellDest) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_S)) && !state->pause) {
        // Alterna o modo de venda
        state->mousePick = (state->mousePick != SELL_ID) ? SELL_ID : 1;
    }

    // Lógica do seletor de personagens
    for (int f = 0; f < 5; f++) {
        Rectangle frameDest = ScaleRectTo720p(300 + (f * 77), 20, 78, 96, screenWidth, screenHeight);
        if (((CheckCollisionPointRec(mousePos, frameDest) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ONE + f)) && state->inCooldown[f] == false && !state->pause && state->characterCost[f] <= state->money) {
            state->shouldPlaySound = true;
            state->soundToPlay = SOUND_SELECT;
            if (state->mousePick != state->frame[f]) {
                state->mousePick = state->frame[f];  // Seleciona o personagem
            } else {
                state->mousePick = 1;
            }
        }

        // Lógica de Cooldown do personagem
        if (state->inCooldown[f] == true && !state->pause) {
            state->frameCounterCD[f] += 96.0f / TimeToFrames(state->characterCD[f]);
            if (state->frameCounterCD[f] >= 96) {
                state->frameCounterCD[f] = 0;
                state->inCooldown[f] = false;
            }
        }
    }

    // Lógica de manipulação de personagens no grid (colocar ou vender)
    HandleCharacterPlacementAndSelling(state, mousePos, screenWidth, screenHeight);

    // Lógica de coleta da bolsa de dinheiro
    if (state->moneyBag) {
        Rectangle moneyBagDest = ScaleRectTo720p(state->randomNumX, state->randomNumY, 78 + state->pisc, 96 + state->pisc, screenWidth, screenHeight);
        if (CheckCollisionPointRec(mousePos, moneyBagDest)) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                state->shouldPlaySound = true;
                state->soundToPlay = SOUND_COLLECTBAG;
                state->money += 25;                  // Adiciona dinheiro
                state->moneyBagsCollected += 1;      // Incrementa o contador de bolsas coletadas
                state->moneyBag = false;             // Remove a bolsa
                state->frameCounterPisc = -2;        // Garante que o timer resete
                state->randomizePointBagPos = true;  // Permite nova randomização
            }
        }
    }
}

// Lógica de posicionamento e venda de personagens no grid.
void HandleCharacterPlacementAndSelling(GameState *state, Vector2 mouse, int screenWidth, int screenHeight) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLUMNS; c++) {
            Rectangle tileDest = ScaleRectTo720p(GRID_MARGIN_X + (c * 96), GRID_MARGIN_Y + (r * 78), 96, 78, screenWidth, screenHeight);

            // Lógica para Chimpanzini brilhando (coletar dinheiro) com a tecla de atalho "C"
            if (state->chimpanzini[r][c].shining == true && IsKeyPressed(KEY_C)) {
                state->chimpanzini[r][c].shining = false;
                state->money += 25;
                state->chimpanzini[r][c].idle = 0;  // Reseta animação
                state->shouldPlaySound = true;
                state->soundToPlay = SOUND_COLLECT;
            }

            // Verifica colisão do mouse com a tile e se o botão esquerdo foi pressionado
            if (CheckCollisionPointRec(mouse, tileDest) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                // Lógica para Chimpanzini brilhando (coletar dinheiro) clicando nele
                if (state->tiles[r][c] == CHIMPANZINI_ID && state->chimpanzini[r][c].shining) {
                    state->chimpanzini[r][c].shining = false;
                    state->money += 25;
                    state->chimpanzini[r][c].idle = 0;  // Reseta animação
                    state->shouldPlaySound = true;
                    state->soundToPlay = SOUND_COLLECT;
                }
                // Lógica de posicionamento de personagem
                else if (state->tiles[r][c] == 1) {  // Se a tile estiver vazia (valor 1)
                    // Verifica se um personagem está selecionado e se o jogador tem dinheiro suficientes
                    if (state->mousePick >= CHIMPANZINI_ID && state->mousePick <= BOMBARDINI_ID &&
                        state->money >= state->characterCost[state->mousePick - CHIMPANZINI_ID]) {
                        state->shouldPlaySound = true;
                        state->soundToPlay = SOUND_PUT;

                        // Inicializa a struct do personagem e o coloca na tile
                        switch (state->mousePick) {
                            case CHIMPANZINI_ID:
                                state->chimpanzini[r][c] = (Chimpanzini){.hp = 20, .idle = 0, .loop = 0, .shining = false, .exists = true};

                                break;
                            case TRALALERO_ID:
                                state->tralalero[r][c] = (Tralalero){.hp = 50, .idle = 0, .loop = 0, .projecX = (GRID_MARGIN_X + 20) + (c * 96) + 35, .projecY = GRID_MARGIN_Y + (r * 78), .projecB = false, .attacking = false, .exists = true};
                                break;
                            case SAHUR_ID:
                                state->sahur[r][c] = (Sahur){.hp = 50, .idle = 0, .cooldown = 0, .attacking = false, .wait = false, .exists = true};
                                break;
                            case LIRILI_ID:
                                state->lirili[r][c] = (Lirili){.hp = 150, .idle = 0, .exists = true};
                                break;
                            case BOMBARDINI_ID:
                                state->bombardini[r][c] = (Bombardini){.hp = 10, .idle = 0, .loop = 0, .bombX = 0, .bombY = 0, .bombB = false, .ready = false, .exists = true};
                                break;
                        }
                        state->charactersBought += 1;                                             // Incrementa o número de personagens comprados
                        state->tiles[r][c] = state->mousePick;                                    // Atualiza o tipo da tile
                        state->inCooldown[state->mousePick - CHIMPANZINI_ID] = true;              // Deixa o personagem em cooldown
                        state->money -= state->characterCost[state->mousePick - CHIMPANZINI_ID];  // Deduz o custo
                        state->mousePick = 1;                                                     // Reseta a seleção do mouse
                    }
                }

                // Lógica de venda de personagem
                else if (state->mousePick == SELL_ID && state->tiles[r][c] != 0 && state->tiles[r][c] != 1) {
                    // Se o modo de venda está ativo e a tile não é vazia nem um botão
                    int characterId = state->tiles[r][c];

                    switch (characterId) {
                        case CHIMPANZINI_ID:
                            state->chimpanzini[r][c].exists = false;
                            break;
                        case TRALALERO_ID:
                            state->tralalero[r][c].exists = false;
                            break;
                        case SAHUR_ID:
                            state->sahur[r][c].exists = false;
                            break;
                        case LIRILI_ID:
                            state->lirili[r][c].exists = false;
                            break;
                        case BOMBARDINI_ID:
                            state->bombardini[r][c].exists = false;
                            state->money += 10;
                            break;  // Bombardini tem um retorno diferente
                    }
                    if (characterId != BOMBARDINI_ID) {  // Adiciona metade do custo de volta, exceto para Bombardini
                        state->money += state->characterCost[characterId - CHIMPANZINI_ID] / 2;
                    }
                    state->shouldPlaySound = true;
                    state->soundToPlay = SOUND_CANCEL;
                    state->charactersSold += 1;  // Incrementa o número de personagens vendidos
                    state->tiles[r][c] = 1;      // Retorna a tile para o estado padrão
                }
            }
        }
    }
}

// Lógica dos botões do menu de pause
void HandlePause(GameState *state, Vector2 mousePos, int screenWidth, int screenHeight) {
    Rectangle option1GlowDest = ScaleRectTo720p(504, (screenHeight / 4) + 24, 312, 121 - 48, screenWidth, screenHeight);
    Rectangle option2GlowDest = ScaleRectTo720p(504, (screenHeight / 2) + 24, 312, 121 - 48, screenWidth, screenHeight);

    if (CheckCollisionPointRec(mousePos, option1GlowDest)) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            state->pause = false;  // Sai do menu de pause
        }
    }
    if (CheckCollisionPointRec(mousePos, option2GlowDest)) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            CloseAudioDevice();  // Fecha o áudio
            CloseWindow();       // Fecha a janela
            exit(0);             // Finaliza o programa
        }
    }
}
