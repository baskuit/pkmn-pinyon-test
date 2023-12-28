/*

This is a model that runs on the BattleTypes::State in this example.


We create a mapped_state version of the state, with some number of chance tries and depth as a parameter to the model

For the model of this napped state, we will use a bandit search model.

The we run full-depth (same as parameter) alpha beta on this state (with obfuscated bandit search model as get_payoff)

Finally we just return the payoff

Thus the template parameter is just ModelTypes


*/

#include <pinyon.hh>

template <CONCEPT(IsModelTypes, Types)>
struct MappedAlphaBetaModel
{
    struct ModelOuput
    {
        typename Types::Value value;
    };

    class Model
    {
    public:
        const size_t depth;
        const size_t tries;
        typename Types::PRNG device;
        typename Types::Model model;

        Model(
            const size_t depth,
            const size_t tries,
            const Types::PRNG &device,
            const Types::Model &model)
            : depth{depth}, tries{tries}, device{device}, model{model}
        {
        }

        void inference(
            const Types::State &&state,
            ModelOuput &output)
        {
            // Types has
            using AB = AlphaBeta<EmptyModel<MappedState<Types>>>;
            typename AB::State mapped_state{
                depth,
                tries,
                device,
                state,
                model};
            AB::Model ab_model{};
            AB::Search search{};
            AB::MatrixNode node{};
            search.run(-1, device, state, model, node);
            // TODO get name right
            output.value = root.stats.payoff_value;
        }
    };
};