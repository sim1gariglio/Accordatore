#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Costanti
// ---------------------------------------------------------------------------
constexpr double PI = 3.14159265358979323846;
constexpr double SAMPLE_RATE = 44100.0;     // campioni al secondo
constexpr std::size_t NUM_CAMPIONI = 2048;  // dimensione di un blocco audio (~46 ms)
constexpr double FREQ_MIN = 20.0;           // sotto: nessuna nota
constexpr double FREQ_MAX = 5000.0;         // sopra: nessuna nota
constexpr float SOGLIA = 0.1f;              // YIN: sotto questo valore c'e' un periodo

// ---------------------------------------------------------------------------
// Da frequenza a nota
// ---------------------------------------------------------------------------
struct Nota {
    std::string nome;
    int ottava{};
    double cent{};
};

const std::array<std::string, 12> nomiNote{
    "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si"};

std::optional<Nota> daFrequenza(double f)
{
    // Scritto "al contrario" così scarta anche i NaN
    if (!(f >= FREQ_MIN && f <= FREQ_MAX))
        return std::nullopt;

    double midi = 12 * std::log2(f / 440.0) + 69;
    int numero = static_cast<int>(std::round(midi));

    Nota n{};
    n.nome = nomiNote[numero % 12];
    n.ottava = numero / 12 - 1;
    n.cent = (midi - numero) * 100;
    return n;
}

// ---------------------------------------------------------------------------
// Segnale di prova (fa la parte del microfono)
// ---------------------------------------------------------------------------
std::vector<float> generaSinusoide(double frequenza, double sampleRate, std::size_t numCampioni)
{
    std::vector<float> ris(numCampioni);
    for (std::size_t i = 0; i < ris.size(); ++i)
        ris[i] = static_cast<float>(std::sin(2 * PI * frequenza * i / sampleRate));
    return ris;
}

// ---------------------------------------------------------------------------
// YIN, passo 1: funzione differenza
// ---------------------------------------------------------------------------
std::vector<float> funzioneDifferenza(const std::vector<float>& campioni)
{
    const std::size_t w = campioni.size() / 2;
    std::vector<float> d(w);
    for (std::size_t tau = 1; tau < w; ++tau) {
        for (std::size_t j = 0; j < w; ++j) {
            float diff = campioni[j] - campioni[j + tau];
            d[tau] += diff * diff;
        }
    }
    return d;
}

// ---------------------------------------------------------------------------
// YIN, passo 2: differenza normalizzata (indipendente dal volume)
// ---------------------------------------------------------------------------
std::vector<float> differenzaNormalizzata(const std::vector<float>& d)
{
    std::vector<float> ris(d.size());
    ris[0] = 1.0f;
    double somma{};
    for (std::size_t tau = 1; tau < d.size(); ++tau) {
        somma += d[tau];
        ris[tau] = static_cast<float>(d[tau] * tau / somma);
    }
    return ris;
}

// ---------------------------------------------------------------------------
// YIN, passo 3: il primo tau sotto la soglia, poi in fondo alla "valle"
// ---------------------------------------------------------------------------
std::optional<std::size_t> trovaPeriodo(const std::vector<float>& dn)
{
    // L'ultimo tau valido e' size - 2: l'interpolazione vuole un vicino a destra
    const std::size_t ultimo = dn.size() - 2;

    for (std::size_t tau = 2; tau <= ultimo; ++tau) {
        if (dn[tau] < SOGLIA) {
            // Siamo entrati nella valle: scendiamo finche' il valore diminuisce
            while (tau + 1 <= ultimo && dn[tau + 1] < dn[tau])
                ++tau;
            return tau;
        }
    }
    return std::nullopt;  // nessun periodo: rumore o silenzio
}

// YIN, passo 4: affina tau con i decimali (tau deve avere un vicino a sinistra e a destra)
double interpolazioneParabolica(const std::vector<float>& d, std::size_t tau)
{
    double sinistra = d[tau - 1];
    double centro = d[tau];
    double destra = d[tau + 1];
    return tau + (sinistra - destra) / (2 * (sinistra - 2 * centro + destra));
}

// ---------------------------------------------------------------------------
// Stampa
// ---------------------------------------------------------------------------
void stampaNota(double frequenza)
{
    std::cout << frequenza << " Hz -> ";
    if (auto risultato = daFrequenza(frequenza)) {
        auto [nome, ottava, cent] = *risultato;
        std::cout << nome << ottava << " " << cent << " cent\n";
    } else {
        std::cout << "nessuna nota\n";
    }
}

// ---------------------------------------------------------------------------
// Tutta la catena: campioni -> frequenza (se c'e')
// ---------------------------------------------------------------------------
std::optional<double> rilevaFrequenza(const std::vector<float>& campioni)
{
    auto d = funzioneDifferenza(campioni);
    auto dn = differenzaNormalizzata(d);
    auto tau = trovaPeriodo(dn);
    if (!tau)
        return std::nullopt;
    double tauPreciso = interpolazioneParabolica(dn, *tau);
    return SAMPLE_RATE / tauPreciso;
}

// ---------------------------------------------------------------------------
int main()
{
    for (double f : {440.0, 82.41, 329.63}) {
        std::cout << "Suono " << f << " Hz: ";
        auto suono = generaSinusoide(f, SAMPLE_RATE, NUM_CAMPIONI);
        if (auto rilevata = rilevaFrequenza(suono))
            stampaNota(*rilevata);
        else
            std::cout << "nessuna nota\n";
    }

    std::cout << "Silenzio: ";
    std::vector<float> silenzio(NUM_CAMPIONI);
    if (auto rilevata = rilevaFrequenza(silenzio))
        stampaNota(*rilevata);
    else
        std::cout << "nessuna nota\n";
}