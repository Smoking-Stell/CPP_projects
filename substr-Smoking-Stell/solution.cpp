#include <iostream>
#include <cstring>


size_t KMP (char newElement, size_t pred, const char* str, const size_t *prefunc) {
    size_t ans = pred;
    while (ans > 0 && newElement != str[ans]) {
        ans = prefunc[ans - 1];
    }
    ans += (newElement == str[ans]);
    return ans;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: Text file name, SubString\n");
        return -1;
    }

    // создание файла и проверка его существования
    FILE* file = fopen(argv[1], "rb");
    if (file == nullptr) {
        fprintf(stderr, strerror(errno), "\n");
        return -2;
    }

    // массив для подсчета перфикс функции подстроки,
    // нужно занулить 0 элемент, потому что там лежит непонятно что
    const char* subStr = argv[2];
    size_t length = strlen(subStr);
    size_t *prefunc = (size_t*) malloc((length + 1) * sizeof(prefunc));
    prefunc[0] = 0;

    // сам КМП для подстроки и меняем последний элемент на нулевой
    for (size_t i = 1; i < length; i++) {
        prefunc[i] = KMP(subStr[i], prefunc[i - 1], subStr, prefunc);
    }
    prefunc[length] = 0;

    // онлайн КМП алгоритм для самого текста
    size_t intc;
    size_t ans = 0;
    bool found = false;
    while ((intc = fgetc(file)) != EOF) {
        ans = KMP((char) intc, ans, subStr, prefunc);
        // проверка, что достигли ответа
        if (ans == length) {
            found = true;
            break;
        }
    }

    if (found) {
        fprintf(stdout, "Yes\n");
        fclose(file);
        free(prefunc);
        return 0;
    }

    // проверка на ошибку при чтении, выходим из цикла при нахождении,
    // поэтому если найдем, то ничего не выведем
    if (ferror(file)) {
        fprintf(stderr, "Read problem\n");
        free(prefunc);
        return -3;
    }

    fprintf(stdout, "No\n");
    fclose(file);
    free(prefunc);
    return 0;
}
